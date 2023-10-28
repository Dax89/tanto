#include "backendimpl.h"
#include <QShortcut>
#include <QScreen>
#include <QAction>
#include <QInputDialog>
#include <QFileDialog>
#include <QScrollArea>
#include <QMessageBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include "../../events.h"
#include "../../tanto.h"
#include "../../utils.h"
#include "../../error.h"
#include "mainwindow.h"
#include "picture.h"

Q_DECLARE_METATYPE(tanto::types::MultiValue);

namespace {

const QString CENTRAL_WIDGET = "__tanto_central_widget__";

[[nodiscard]] QObject* qtcontainer_cast(const std::any& arg) {
    if(arg.type() == typeid(QWidget*)) return std::any_cast<QWidget*>(arg);
    else if(arg.type() == typeid(MainWindow*)) return std::any_cast<MainWindow*>(arg);
    else if(arg.type() == typeid(QVBoxLayout*)) return std::any_cast<QVBoxLayout*>(arg);
    else if(arg.type() == typeid(QHBoxLayout*)) return std::any_cast<QHBoxLayout*>(arg);
    else if(arg.type() == typeid(QGridLayout*)) return std::any_cast<QGridLayout*>(arg);
    else if(arg.type() == typeid(QFormLayout*)) return std::any_cast<QFormLayout*>(arg);
    else if(arg.type() == typeid(QTabWidget*)) return std::any_cast<QTabWidget*>(arg);
    else if(arg.type() == typeid(QGroupBox*)) return std::any_cast<QGroupBox*>(arg);

    except("Unsupported container type: '{}'", arg.type().name());
}

template<typename T>
T* apply_parent(T* arg, QObject* parent, const tanto::types::Widget& w) {
    if constexpr(std::is_base_of_v<QLayout, T>) {
        if(MainWindow* mw = qobject_cast<MainWindow*>(parent); mw) mw->setLayout(arg);
        else if(QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(parent); vbox) vbox->addLayout(arg, w.fill);
        else if(QHBoxLayout* hbox = qobject_cast<QHBoxLayout*>(parent); hbox) hbox->addLayout(arg, w.fill);
        else if(QGridLayout* grid = qobject_cast<QGridLayout*>(parent); grid) grid->addLayout(arg, w.prop<int>("row"), w.prop<int>("col"), w.prop<int>("rowspan", 1), w.prop<int>("colspan", 1));
        else if(QFormLayout* form = qobject_cast<QFormLayout*>(parent); form) form->addRow(QString::fromStdString(w.prop<std::string>("label")), arg);
        else if(QTabWidget* tabs = qobject_cast<QTabWidget*>(parent); tabs) {
            QWidget* c = new QWidget(); // Create container for this layout
            c->setLayout(arg);
            tabs->addTab(c, QString::fromStdString(w.prop<std::string>("label")));
        }
        else if(QWidget* widget = qobject_cast<QWidget*>(parent); widget) widget->setLayout(arg);
        else unreachable;
    }
    else {
        if(QWidget* widget = qobject_cast<QWidget*>(parent); widget && widget->objectName() == CENTRAL_WIDGET)
            parent = widget->parent();

        if(MainWindow* mw = qobject_cast<MainWindow*>(parent); mw) {
            QWidget* oldwidget = mw->centralWidget();
            mw->setCentralWidget(arg);
            if(oldwidget) oldwidget->deleteLater();
        }
        else if(QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(parent); vbox) vbox->addWidget(arg, w.fill);
        else if(QHBoxLayout* hbox = qobject_cast<QHBoxLayout*>(parent); hbox) hbox->addWidget(arg, w.fill);
        else if(QGridLayout* grid = qobject_cast<QGridLayout*>(parent); grid) grid->addWidget(arg, w.prop<int>("row"), w.prop<int>("col"), w.prop<int>("rowspan", 1), w.prop<int>("colspan", 1));
        else if(QFormLayout* form = qobject_cast<QFormLayout*>(parent); form) form->addRow(QString::fromStdString(w.prop<std::string>("label")), arg);
        else if(QTabWidget* tabs = qobject_cast<QTabWidget*>(parent); tabs) tabs->addTab(arg, QString::fromStdString(w.prop<std::string>("label")));
        else unreachable;
    }

    return arg;
}

QString convert_filter(const tanto::FilterList& filters)
{
    QStringList res;

    for(const tanto::Filter& f : filters)
    {
        QStringList ext;

        for(const std::string& e : f.ext)
        {
            if(e != "*")
                ext.push_back(QString::fromStdString(e).prepend("*."));
            else
                ext.push_back("*");
        }

        res.push_back(QString("%1 (%2)").arg(QString::fromStdString(f.name))
                                        .arg(ext.join(' ')));
    }

    return res.join(";;");
}

template<typename Slot>
QAction* qtadd_action(QWidget* w, const QString& text, const QKeySequence& shortcut, QObject* object, Slot slot)
{
    QAction* action = new QAction(text, w);
    action->setShortcut(shortcut);
    w->addAction(action);

    QObject::connect(action, &QAction::triggered, object, slot);
    return action;
}

[[nodiscard]] tanto::types::MultiValue qttreeindex_getmultivalue(QTreeWidget* tree, QModelIndex index)
{
    QTreeWidgetItem* item = tree->itemFromIndex(index);
    assume(item);

    QVariant v = item->data(0, Qt::UserRole);
    assume(!v.isNull());
    return v.value<tanto::types::MultiValue>();
}

[[nodiscard]] std::string qttreeindex_getid(QTreeWidget* tree, QModelIndex index)
{
    tanto::types::MultiValue v = qttreeindex_getmultivalue(tree, index);
    std::string id;

    std::visit(tanto::utils::Overload{
             [&](tanto::types::Widget& a) { id = a.get_id(); },
             [&](std::string& a) { id = a; }
    }, v);

    return id;
}

[[nodiscard]] QTreeWidgetItem* qttree_add(QTreeWidget* tree, tanto::types::MultiValue& item)
{
    QTreeWidgetItem* treeitem = new QTreeWidgetItem();
    treeitem->setFlags(treeitem->flags() | Qt::ItemIsSelectable);

    QVariant v;
    v.setValue(item);
    treeitem->setData(0, Qt::UserRole, v);

    std::visit(tanto::utils::Overload{
             [&](tanto::types::Widget& a) {
                  treeitem->setText(0, QString::fromStdString(a.text)); 
                  if(a.prop<bool>("selected")) tree->setCurrentItem(treeitem);

                  for(tanto::types::MultiValue childitem : a.items)
                      treeitem->addChild(qttree_add(tree, childitem));
             },
             [&](std::string& a) { treeitem->setText(0, QString::fromStdString(a)); }
    }, item);

    return treeitem;
}

} // namespace

BackendQtImpl::BackendQtImpl(int& argc, char* argv[]): Backend{argc, argv}, m_app{argc, argv} { }

BackendQtImpl::~BackendQtImpl()
{
    if(m_mainwindow) m_mainwindow->deleteLater();
    m_mainwindow = nullptr;
}

std::string_view BackendQtImpl::version() { return QT_VERSION_STR; }
int BackendQtImpl::run() const { return m_app.exec(); }

std::any BackendQtImpl::new_window(const tanto::types::Window& arg)
{
    MainWindow* mw = new MainWindow();
    mw->setWindowTitle(QString::fromStdString(arg.title));
    mw->setGeometry(arg.x, arg.y, arg.width, arg.height);

    if(arg.type == "popup") mw->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup | Qt::NoDropShadowWindowHint);
    else if(arg.type == "tool") mw->setWindowFlags(Qt::Tool);

    QAction* act = qtadd_action(mw, QString{}, QKeySequence{Qt::Key_Escape}, qApp, &QApplication::exit);
    act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    
    if(!arg.x && !arg.y) // Center window
    {
        QRect position = mw->frameGeometry();
        position.moveCenter(qApp->primaryScreen()->availableGeometry().center());
        mw->move(position.topLeft());
    }

    if(arg.fixed) mw->setFixedSize(arg.width, arg.height);

    if(auto f = tanto::parse_font(arg.font); f)
    {
        QFont font{QString::fromStdString(f->first)};
        if(f->second != -1) font.setPointSize(f->second);
        mw->setFont(font);
    }

    QWidget* body = new QWidget();
    body->setObjectName(CENTRAL_WIDGET);
    mw->setCentralWidget(body);

    mw->show();

    m_mainwindow = mw;
    return body;
}

void BackendQtImpl::exit() { qApp->quit(); }

void BackendQtImpl::message(const std::string& title, const std::string& text, MessageType mt, MessageIcon icon)
{
    QMessageBox::Icon mbicon = QMessageBox::NoIcon;

    switch(icon)
    {
        case MessageIcon::INFO:     mbicon = QMessageBox::Information; break;
        case MessageIcon::WARNING:  mbicon = QMessageBox::Warning; break;
        case MessageIcon::QUESTION: mbicon = QMessageBox::Question; break;
        case MessageIcon::ERROR:    mbicon = QMessageBox::Critical; break;
        default: break;
    }

    QMessageBox msgbox{mbicon, QString::fromStdString(title), QString::fromStdString(text)};

    if(mt == MessageType::MESSAGE) msgbox.setStandardButtons(QMessageBox::Ok);
    else if(mt == MessageType::CONFIRM) msgbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    else unreachable;

    std::string response;

    switch(msgbox.exec())
    {
        case QMessageBox::Ok: this->send_event("ok"); break;
        case QMessageBox::Cancel: this->send_event("cancel"); break;
        case QMessageBox::NoButton: break;
        default: unreachable;
    }
}

void BackendQtImpl::input(const std::string& title, const std::string& text, const std::string& value, InputType it)
{
    bool ok = false;
    QLineEdit::EchoMode echomode = it == InputType::PASSWORD ? QLineEdit::Password : QLineEdit::Normal;

    QString res = QInputDialog::getText(nullptr, QString::fromStdString(title),
                                        QString::fromStdString(text), echomode,
                                        QString::fromStdString(value), &ok);

    this->send_event(ok ? res.toStdString() : std::string{});
}

void BackendQtImpl::load_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir)
{
    QString filepath = QFileDialog::getOpenFileName(nullptr,
                                                    title.empty() ? QString{} : QString::fromStdString(title),
                                                    startdir.empty() ? QString{} : QString::fromStdString(startdir),
                                                    convert_filter(filter));

    this->send_event(filepath.toStdString());
}

void BackendQtImpl::save_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir)
{
    QString filepath = QFileDialog::getSaveFileName(nullptr,
                                                    title.empty() ? QString{} : QString::fromStdString(title),
                                                    startdir.empty() ? QString{} : QString::fromStdString(startdir),
                                                    convert_filter(filter));

    this->send_event(filepath.toStdString());
}

void BackendQtImpl::select_dir(const std::string& title, const std::string& startdir)
{
    QString dir = QFileDialog::getExistingDirectory(nullptr,
                                                    title.empty() ? QString{} : QString::fromStdString(title),
                                                    startdir.empty() ? QString{} : QString::fromStdString(startdir),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    this->send_event(dir.toStdString());
}

std::any BackendQtImpl::new_space(const tanto::types::Widget& arg, const std::any& parent)
{
    (void)arg;

    QObject* p = qtcontainer_cast(parent);
    assume(qobject_cast<QBoxLayout*>(p));
    qobject_cast<QBoxLayout*>(p)->addStretch();
    return nullptr;
}

std::any BackendQtImpl::new_text(const tanto::types::Widget& arg, const std::any& parent)
{
    QLabel* w = new QLabel(QString::fromStdString(arg.text));
    w->setEnabled(arg.enabled);
    return apply_parent(w, qtcontainer_cast(parent), arg);
}

std::any BackendQtImpl::new_input(const tanto::types::Widget& arg, const std::any& parent)
{
    QLineEdit* w = new QLineEdit();
    w->setEnabled(arg.enabled);
    w->setText(QString::fromStdString(arg.text));
    w->setPlaceholderText(QString::fromStdString(arg.prop<std::string>("placeholder")));
    return apply_parent(w, qtcontainer_cast(parent), arg);
}

std::any BackendQtImpl::new_number(const tanto::types::Widget& arg, const std::any& parent)
{
    QSpinBox* w = new QSpinBox();
    w->setEnabled(arg.enabled);
    w->setSingleStep(arg.prop<int>("step", 1));

    w->setRange(arg.prop<int>("min", tanto::NUMBER_MIN),
                arg.prop<int>("max", tanto::NUMBER_MAX));


    w->setValue(arg.value);
    return apply_parent(w, qtcontainer_cast(parent), arg);
}

std::any BackendQtImpl::new_image(const tanto::types::Widget& arg, const std::any& parent)
{
    Picture* w = new Picture();
    w->setImageSize(arg.width, arg.height);
    if(!arg.text.empty()) w->loadImage(arg.text);
    apply_parent(w, qtcontainer_cast(parent), arg);

    if(arg.has_id())
    {
        QObject::connect(w, &Picture::doubleClicked, w, [&, arg, w]() {
            this->double_clicked(arg, w->filePath().toStdString());
        });
    }

    return w;
}

std::any BackendQtImpl::new_button(const tanto::types::Widget& arg, const std::any& parent)
{
    QPushButton* w = new QPushButton(QString::fromStdString(arg.text));
    w->setEnabled(arg.enabled);
    apply_parent(w, qtcontainer_cast(parent), arg);

    if(arg.has_id())
    {
        QObject::connect(w, &QPushButton::clicked, w, [&, arg]() {
            this->clicked(arg);
        });
    }

    return w;
}

std::any BackendQtImpl::new_check(const tanto::types::Widget& arg, const std::any& parent)
{
    QCheckBox* w = new QCheckBox(QString::fromStdString(arg.text));
    w->setEnabled(arg.enabled);
    w->setChecked(arg.prop<bool>("checked"));
    apply_parent(w, qtcontainer_cast(parent), arg);

    if(arg.has_id())
    {
        QObject::connect(w, &QCheckBox::stateChanged, w, [&, arg](int state) {
            this->changed(arg, state == Qt::Checked);
        });
    }

    return w;
}

std::any BackendQtImpl::new_list(const tanto::types::Widget& arg, const std::any& parent)
{
    QListWidget* w = new QListWidget();
    w->setEnabled(arg.enabled);

    for(auto item : arg.items)
    {
        std::visit(tanto::utils::Overload{
                [&](tanto::types::Widget& a) {
                    w->addItem(QString::fromStdString(a.text));
                    if(a.prop<bool>("selected")) w->setCurrentRow(w->count() - 1);
                },
                [&](std::string& a) { w->addItem(QString::fromStdString(a)); }
        }, item);
    }

    apply_parent(w, qtcontainer_cast(parent), arg);

    if(arg.has_id())
    {
        auto itemselected = [&, arg](const QModelIndex& index) {
            this->selected(arg, index.row(), arg.items[index.row()]);
        };

        qtadd_action(w, QString{}, QKeySequence{Qt::Key_Return}, w, [w, itemselected]() {
            if(w->currentIndex().isValid()) itemselected(w->currentIndex());
        });

        QObject::connect(w, &QListWidget::doubleClicked, w, itemselected);
    }

    return w;
}

std::any BackendQtImpl::new_tree(const tanto::types::Widget& arg, const std::any& parent)
{
    QTreeWidget* w = new QTreeWidget();
    w->setSelectionMode(QTreeWidget::SingleSelection);
    w->setSelectionBehavior(QTreeWidget::SelectRows);
    w->setHeaderHidden(true);
    w->setEnabled(arg.enabled);

    apply_parent(w, qtcontainer_cast(parent), arg);

    for(tanto::types::MultiValue item : arg.items)
        w->addTopLevelItem(qttree_add(w, item));
    
    if(arg.has_id())
    {
        auto itemselected = [&, w, arg](const QModelIndex& index) {
            tanto::types::MultiValue v = qttreeindex_getmultivalue(w, index);
            nlohmann::json detail = nlohmann::json::object();

            if(index.parent().isValid()) {
                detail["parentid"] = qttreeindex_getid(w, index.parent());
                detail["parentindex"] = index.parent().row();
            } 

            this->selected(arg, index.row(), v, detail);
        };

        qtadd_action(w, QString{}, QKeySequence{Qt::Key_Return}, w, [w, itemselected]() {
            if(w->currentItem()) itemselected(w->currentIndex());
        });

        QObject::connect(w, &QListWidget::doubleClicked, w, itemselected);
    }

    return w;
}

std::any BackendQtImpl::new_tabs(const tanto::types::Widget& arg, const std::any& parent) { return apply_parent(new QTabWidget(), qtcontainer_cast(parent), arg); }
std::any BackendQtImpl::new_row(const tanto::types::Widget& arg, const std::any& parent) { return apply_parent(new QHBoxLayout(), qtcontainer_cast(parent), arg); }
std::any BackendQtImpl::new_column(const tanto::types::Widget& arg, const std::any& parent) { return apply_parent(new QVBoxLayout(), qtcontainer_cast(parent), arg); }
std::any BackendQtImpl::new_grid(const tanto::types::Widget& arg, const std::any& parent) { return apply_parent(new QGridLayout(), qtcontainer_cast(parent), arg); }

std::any BackendQtImpl::new_form(const tanto::types::Widget& arg, const std::any& parent)
{
    QFormLayout* w = new QFormLayout();
    w->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    return apply_parent(w, qtcontainer_cast(parent), arg);
}

std::any BackendQtImpl::new_group(const tanto::types::Widget& arg, const std::any& parent) { return apply_parent(new QGroupBox(QString::fromStdString(arg.text)), qtcontainer_cast(parent), arg); }
