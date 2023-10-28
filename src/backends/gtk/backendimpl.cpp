#include "backendimpl.h"
#include <fmt/core.h>
#include "../../tanto.h"
#include "../../utils.h"
#include "../../error.h"

namespace {

const std::string FORM_WIDGET    = "__tanto_form_widget__";
const std::string SPACE_WIDGET   = "__tanto_space_widget__";
constexpr guint DEFAULT_SPACING  = 5;

struct ImageInfo
{
    tanto::types::Widget twidget;
    GdkPixbuf* pixbuf;
    GtkWidget* widget;
    std::string filepath;
};

struct WidgetInfo
{
    BackendGtkImpl* self;
    tanto::types::Widget twidget;
};

std::unordered_map<GtkWidget*, WidgetInfo> g_widgets;
std::unordered_map<GtkWidget*, ImageInfo> g_images;
std::unordered_map<GtkWidget*, int> g_nrows;

[[nodiscard]] GtkWidget* gtkcreate_label(const std::string& text, gfloat xalign = 0.0)
{
    GtkWidget* w = gtk_label_new(text.c_str());
    gtk_label_set_xalign(GTK_LABEL(w), xalign);
    return w;
}

[[nodiscard]] GtkWidget* gtkcreate_grid()
{
    GtkWidget* w = gtk_grid_new();
    g_nrows[w] = 0;
    return w;
}

[[nodiscard]] GtkWidget* gtkcontainer_cast(const std::any& arg) {
    if(arg.type() == typeid(GtkWidget*)) return std::any_cast<GtkWidget*>(arg);
    else except("Unsupported container type: '{}'", arg.type().name());
}

void apply_parent(GtkWidget* arg, GtkWidget* parent, const tanto::types::Widget& w) {
    if(GTK_IS_GRID(parent)) {
        GtkGrid* grid = GTK_GRID(parent);

        if(gtk_widget_get_name(parent) == FORM_WIDGET) {
            gtk_grid_attach(grid, 
                            gtkcreate_label(w.prop<std::string>("label").c_str(), 1.0),
                            0, g_nrows[parent], 1, 1);

            gtk_grid_attach(grid, arg, 1, g_nrows[parent], 1, 1);
            ++g_nrows[parent];
        }
        else {

        }
    }
    else if(GTK_IS_NOTEBOOK(parent)) {
        GtkNotebook* notebook = GTK_NOTEBOOK(parent);

        gtk_notebook_append_page(notebook, 
                                 arg, 
                                 gtkcreate_label(w.prop<std::string>("label").c_str()));
    }
    else if(GTK_IS_BOX(parent))
    {
        bool isspace = std::string_view{gtk_widget_get_name(arg)} == SPACE_WIDGET;
        bool fill = (isspace || GTK_IS_SCROLLED_WINDOW(arg)) ? true : w.fill;
        gtk_box_pack_start(GTK_BOX(parent), arg, fill, fill, 0);
    }
    else if(GTK_IS_CONTAINER(parent)) gtk_container_add(GTK_CONTAINER(parent), arg);
    else except("Unsupported container type");
}

GtkWidget* setup_widget(GtkWidget* w, const tanto::types::Widget& arg, const std::any& parent)
{
    gtk_widget_set_sensitive(w, arg.enabled);
    gtk_widget_set_size_request(w, arg.width, arg.height);
    apply_parent(w, gtkcontainer_cast(parent), arg);
    return w;
}

void destroy_image(GtkWidget* widget, gpointer)
{
    assume(g_images.count(widget));
    const ImageInfo& imageinfo = g_images[widget];

    g_object_unref(imageinfo.pixbuf);
    g_images.erase(widget);
}

gboolean resize_image(GtkWidget *widget, GdkRectangle *allocation, gpointer /* userdata */)
{
    assume(g_images.count(widget));

    const ImageInfo& imageinfo = g_images[widget];
    double ratio = gdk_pixbuf_get_height(imageinfo.pixbuf) / static_cast<double>(gdk_pixbuf_get_width(imageinfo.pixbuf));
    int w{}, h{};

    if(imageinfo.twidget.width)
    {
        w = imageinfo.twidget.width;
        h = std::ceil(imageinfo.twidget.width * ratio);
    }
    else if(imageinfo.twidget.height)
    {
        h = imageinfo.twidget.height;
        w = std::ceil(imageinfo.twidget.height * ratio);
    }
    else
    { 
        w = allocation->width;
        h = std::ceil(allocation->width * ratio);
    }

    GdkPixbuf* pxbscaled = gdk_pixbuf_scale_simple(imageinfo.pixbuf, w, h, GDK_INTERP_BILINEAR);

    gtk_image_set_from_pixbuf(GTK_IMAGE(widget), pxbscaled);
    g_object_unref(pxbscaled);
    return false;
}

[[nodiscard]] std::string gtktree_createpath(const std::string& lhs, size_t rhs)
{
    if(lhs.empty()) return std::to_string(rhs);
    return lhs + ":" + std::to_string(rhs);
}

void gtktree_add(GtkTreeStore* model, tanto::types::MultiValueList& items, std::vector<std::string>& selections, GtkTreeIter* parent = nullptr, const std::string& path = {})
{
    for(size_t i = 0; i < items.size(); i++)
    {
        tanto::types::MultiValue& item = items[i];

        GtkTreeIter iter;
        gtk_tree_store_append(model, &iter, parent);

        std::visit(tanto::utils::Overload{
                [&](tanto::types::Widget& a) {
                    gtk_tree_store_set(model, &iter, 0, a.text.c_str(), -1); 
                    std::string currpath = gtktree_createpath(path, i);
                    if(a.prop<bool>("selected")) selections.push_back(currpath);
                    gtktree_add(model, a.items, selections, &iter, currpath);
                },
                [&](std::string& a) { gtk_tree_store_set(model, &iter, 0, a.c_str(), -1); }
        }, item);
    }
}

} // namespace

BackendGtkImpl::BackendGtkImpl(int& argc, char* argv[]): Backend{argc, argv} { gtk_init(&argc, &argv); }
BackendGtkImpl::~BackendGtkImpl() { }

std::string_view BackendGtkImpl::version() { 
    static const std::string VERSION = fmt::format("{}.{}.{}", GTK_MAJOR_VERSION, 
                                                               GTK_MINOR_VERSION,
                                                               GTK_MICRO_VERSION);
    return VERSION;
}

int BackendGtkImpl::run() const { gtk_main(); return 0; }

void BackendGtkImpl::exit()
{ 
    g_object_unref(G_OBJECT(m_mainwindow));
    gtk_main_quit();
}

void BackendGtkImpl::widget_processed(const tanto::types::Widget& arg, const std::any& widget)
{
    if(!arg.has_id()) return;
    g_widgets[std::any_cast<GtkWidget*>(widget)] = {this, arg};
}

void BackendGtkImpl::processed() { gtk_widget_show_all(m_mainwindow); }

void BackendGtkImpl::filechooser_show(GtkFileChooserAction action, const std::string& title, const tanto::FilterList& filter, const std::string& startdir)
{
    GtkWidget* w = gtk_file_chooser_dialog_new(title.c_str(), nullptr, action, 
                                               "_Open", GTK_RESPONSE_ACCEPT,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               nullptr);

    if(!filter.empty())
    {
        GtkFileFilter* filefilter = gtk_file_filter_new();

        for(const tanto::Filter& f : filter)
        {
            gtk_file_filter_set_name(filefilter, f.name.c_str());

            for(const std::string& ext : f.ext)
                gtk_file_filter_add_pattern(filefilter, ext.c_str());
        }

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(w), filefilter);
    }

    if(!startdir.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), startdir.c_str());

    gint res = gtk_dialog_run(GTK_DIALOG(w));

    if(res == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w));
        this->send_event(filename);
        g_free(filename);
    }
}


std::any BackendGtkImpl::new_window(const tanto::types::Window& arg)
{
    m_mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(m_mainwindow), "destroy", gtk_main_quit, nullptr);

    gtk_window_set_title(GTK_WINDOW(m_mainwindow), arg.title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(m_mainwindow), arg.width, arg.height);
    gtk_window_set_resizable(GTK_WINDOW(m_mainwindow), !arg.fixed);

    if(!arg.x && !arg.y)
        gtk_window_set_position(GTK_WINDOW(m_mainwindow), GTK_WIN_POS_CENTER_ALWAYS);
    else
        gtk_window_move(GTK_WINDOW(m_mainwindow), arg.x, arg.y);

    gtk_window_present(GTK_WINDOW(m_mainwindow));
    return m_mainwindow;
}

void BackendGtkImpl::message(const std::string& title, const std::string& text, MessageType mt, MessageIcon icon)
{
    std::string_view mbicon;

    switch(icon)
    {
        case MessageIcon::INFO:     mbicon = "dialog-information"; break;
        case MessageIcon::WARNING:  mbicon = "dialog-warning"; break;
        case MessageIcon::QUESTION: mbicon = "dialog-question"; break;
        case MessageIcon::ERROR:    mbicon = "dialog-error"; break;
        default: break;
    }

    GtkWidget* w = nullptr;

    if(mt == MessageType::MESSAGE)
    {
        w = gtk_dialog_new_with_buttons(title.c_str(), nullptr, GTK_DIALOG_MODAL,
                                        "_OK", GTK_RESPONSE_OK,
                                        nullptr);
    }
    else if(mt == MessageType::CONFIRM)
    {
        w = gtk_dialog_new_with_buttons(title.c_str(), nullptr, GTK_DIALOG_MODAL,
                                        "_OK", GTK_RESPONSE_OK,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        nullptr);
    }
    else
        unreachable;

    gtk_container_set_border_width(GTK_CONTAINER(w), DEFAULT_SPACING);
    gtk_window_set_title(GTK_WINDOW(w), title.c_str());

    GtkWidget* vbox = gtk_dialog_get_content_area(GTK_DIALOG(w));
    gtk_box_set_spacing(GTK_BOX(vbox), DEFAULT_SPACING);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DEFAULT_SPACING);

    if(!mbicon.empty())
    {
        gtk_container_add(GTK_CONTAINER(box), gtk_image_new_from_icon_name(mbicon.data(),
                                                                           GTK_ICON_SIZE_DND));
    }

    gtk_container_add(GTK_CONTAINER(box), gtk_label_new(text.c_str()));
    gtk_container_add(GTK_CONTAINER(vbox), box);
    gtk_widget_show_all(GTK_WIDGET(w));

    gint response = gtk_dialog_run(GTK_DIALOG(w));

    switch(response)
    {
        case GTK_RESPONSE_OK: this->send_event("ok"); break;
        case GTK_RESPONSE_CANCEL: this->send_event("cancel"); break;
        default: unreachable;
    }

    gtk_widget_destroy(w);
}

void BackendGtkImpl::input(const std::string& title, const std::string& text, const std::string& value, InputType it)
{
    GtkWidget* w = gtk_dialog_new_with_buttons(title.c_str(), nullptr, GTK_DIALOG_MODAL,
                                               "_OK", GTK_RESPONSE_OK,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               nullptr);

    GtkWidget* vbox = gtk_dialog_get_content_area(GTK_DIALOG(w));
    gtk_box_set_spacing(GTK_BOX(vbox), DEFAULT_SPACING);

    gtk_box_pack_start(GTK_BOX(vbox), gtkcreate_label(text.c_str()), false, false, 0); 

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
    if(!value.empty()) gtk_entry_set_text(GTK_ENTRY(entry), value.c_str());
    gtk_entry_set_visibility(GTK_ENTRY(entry), it == InputType::NORMAL);
    gtk_box_pack_start(GTK_BOX(vbox), entry, false, false, 0);

    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(w), DEFAULT_SPACING);
    gtk_dialog_set_default_response(GTK_DIALOG(w), GTK_RESPONSE_OK);
    gtk_widget_show_all(vbox);

    gint response = gtk_dialog_run(GTK_DIALOG(w));
    if(response == GTK_RESPONSE_OK) this->send_event(gtk_entry_get_text(GTK_ENTRY(entry)));

    gtk_widget_destroy(w);
}

void BackendGtkImpl::load_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir)
{
    this->filechooser_show(GTK_FILE_CHOOSER_ACTION_OPEN, title, filter, startdir);
}

void BackendGtkImpl::save_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir)
{
    this->filechooser_show(GTK_FILE_CHOOSER_ACTION_SAVE, title, filter, startdir);
}

void BackendGtkImpl::select_dir(const std::string& title, const std::string& startdir)
{
    this->filechooser_show(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, title, {}, startdir);
}

std::any BackendGtkImpl::new_space(const tanto::types::Widget& arg, const std::any& parent)
{
    GtkWidget* w = gtk_label_new(nullptr);
    gtk_widget_set_name(w, SPACE_WIDGET.c_str());
    setup_widget(w, arg, parent);
    return nullptr;
}

std::any BackendGtkImpl::new_text(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtkcreate_label(arg.text), arg, parent); }

std::any BackendGtkImpl::new_input(const tanto::types::Widget& arg, const std::any& parent)
{
    GtkWidget* w = gtk_entry_new();
    if(arg.has_prop("placeholder")) gtk_entry_set_placeholder_text(GTK_ENTRY(w), arg.prop<std::string>("placeholder").c_str());
    if(!arg.text.empty()) gtk_entry_set_text(GTK_ENTRY(w), arg.text.c_str());
    setup_widget(w, arg, parent);
    return w;
}

std::any BackendGtkImpl::new_number(const tanto::types::Widget& arg, const std::any& parent)
{
    GtkWidget* w = gtk_spin_button_new_with_range(arg.prop<int>("min", tanto::NUMBER_MIN),
                                                  arg.prop<int>("max", tanto::NUMBER_MAX),
                                                  arg.prop<int>("step", 1));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), arg.value);
    setup_widget(w, arg, parent);
    return w;
}

std::any BackendGtkImpl::new_image(const tanto::types::Widget& arg, const std::any& parent)
{
    std::string filepath = tanto::download_file(arg.text);
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(filepath.c_str(), nullptr);
    assume(pixbuf);

    GtkWidget* w = gtk_image_new();
    g_images[w] = ImageInfo{arg, pixbuf, w, filepath};

    GtkWidget* eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), w);

    g_signal_connect(w, "size-allocate", G_CALLBACK(resize_image), nullptr);
    g_signal_connect(w, "destroy", G_CALLBACK(destroy_image), nullptr);

    if(arg.has_id())
    {
        g_signal_connect(eventbox, "button-press-event", G_CALLBACK(+[](GtkWidget* sender, GdkEventButton* event, BackendGtkImpl* self) {
            if(event->button != 1) return;

            GtkWidget* image = gtk_bin_get_child(GTK_BIN(sender));
            if(event->type == GDK_2BUTTON_PRESS) self->double_clicked(g_widgets.at(sender).twidget, g_images.at(image).filepath);

        }), this);
    }

    setup_widget(eventbox, arg, parent);
    return eventbox;
}

std::any BackendGtkImpl::new_button(const tanto::types::Widget& arg, const std::any& parent)
{ 
    GtkWidget* w = gtk_button_new_with_label(arg.text.c_str());

    if(arg.has_id())
    {
        g_signal_connect(w, "clicked", G_CALLBACK(+[](GtkWidget* sender, BackendGtkImpl* self) {
            self->clicked(g_widgets.at(sender).twidget);
        }), this);
    }

    return setup_widget(w, arg, parent);
}

std::any BackendGtkImpl::new_check(const tanto::types::Widget& arg, const std::any& parent)
{
    GtkWidget* w = gtk_check_button_new_with_label(arg.text.c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), arg.prop<bool>("checked"));

    if(arg.has_id())
    {
        g_signal_connect(w, "clicked", G_CALLBACK(+[](GtkButton* sender, BackendGtkImpl* self) {
            self->changed(g_widgets.at(GTK_WIDGET(sender)).twidget, static_cast<bool>(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sender))));
        }), this);
    }

    return setup_widget(w, arg, parent);
}

std::any BackendGtkImpl::new_list(const tanto::types::Widget& arg, const std::any& parent)
{
    GtkWidget* w = gtk_list_box_new();
    gint i = 0;

    for(auto item : arg.items)
    {
        std::visit(tanto::utils::Overload{
                [&](tanto::types::Widget& a) {
                    gtk_list_box_insert(GTK_LIST_BOX(w), gtkcreate_label(a.text), -1);

                    if(a.prop<bool>("selected")) {
                        GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(w), i);
                        assume(row);
                        gtk_list_box_select_row(GTK_LIST_BOX(w), row);
                    }
                },
                [&](std::string& a) { gtk_list_box_insert(GTK_LIST_BOX(w), gtkcreate_label(a), -1); }
        }, item);

        ++i;
    }

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scroll), w);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    if(arg.has_id())
    {
        g_signal_connect(w, "button-press-event", G_CALLBACK(+[](GtkWidget* sender, GdkEventButton* event, GtkWidget* s) {
            if(event->button != 1 || event->type != GDK_2BUTTON_PRESS) return;

            GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(sender));
            if(!row) return;

            int idx = gtk_list_box_row_get_index(row);
            if(idx == -1) return;

            g_widgets.at(s).self->selected(g_widgets.at(s).twidget, idx, g_widgets.at(s).twidget.items[idx]);
        }), scroll);
    }

    return setup_widget(scroll, arg, parent);
}

std::any BackendGtkImpl::new_tree(const tanto::types::Widget& arg, const std::any& parent)
{
    std::vector<std::string> selections;
    GtkTreeStore* model = gtk_tree_store_new(1, G_TYPE_STRING);
    gtktree_add(model, const_cast<tanto::types::MultiValueList&>(arg.items), selections);

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkWidget* w = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w), false);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(w),
                                                -1,
                                                nullptr,
                                                renderer,
                                                "text", 0,
                                                nullptr);

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scroll), w);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    if(!selections.empty())
    {
        GtkTreePath* treepath = gtk_tree_path_new_from_string(selections.back().c_str());
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(w), treepath);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(w), treepath, nullptr, true, 0.5, 0.0);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(w), treepath, nullptr, false);
        gtk_tree_path_free(treepath);
    }

    return setup_widget(scroll, arg, parent);
}

std::any BackendGtkImpl::new_tabs(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtk_notebook_new(), arg, parent); }
std::any BackendGtkImpl::new_row(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, arg.prop<gint>("spacing", DEFAULT_SPACING)), arg, parent); }
std::any BackendGtkImpl::new_column(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtk_box_new(GTK_ORIENTATION_VERTICAL, arg.prop<gint>("spacing", DEFAULT_SPACING)), arg, parent); }
std::any BackendGtkImpl::new_grid(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtkcreate_grid(), arg, parent); }

std::any BackendGtkImpl::new_form(const tanto::types::Widget& arg, const std::any& parent)
{ 
    GtkWidget* w = gtkcreate_grid();
    gtk_grid_set_column_spacing(GTK_GRID(w), DEFAULT_SPACING);
    gtk_widget_set_name(w, FORM_WIDGET.c_str());
    return setup_widget(w, arg, parent);
}

std::any BackendGtkImpl::new_group(const tanto::types::Widget& arg, const std::any& parent) { return setup_widget(gtk_frame_new(arg.text.c_str()), arg, parent); }
