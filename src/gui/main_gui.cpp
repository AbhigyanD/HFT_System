#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include "main_window.h"

static void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    const QColor bg(0x16, 0x19, 0x1f), panel(0x1e, 0x22, 0x2a), text(0xe6, 0xe9, 0xef), dim(0x8a, 0x92, 0xa0);
    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel);
    p.setColor(QPalette::AlternateBase, QColor(0x23, 0x28, 0x31));
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::PlaceholderText, dim);
    p.setColor(QPalette::Button, QColor(0x2a, 0x30, 0x3b));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::ToolTipBase, QColor(0x2a, 0x30, 0x3b));
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Highlight, QColor(0x4c, 0x9b, 0xff));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text, dim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, dim);
    app.setPalette(p);
    app.setStyleSheet(
        "#sidebar { background:#1e222a; border-right:1px solid #2a303b; }"
        "#card { background:#1e222a; border:1px solid #2a303b; border-radius:8px; }"
        "QGroupBox { border:1px solid #2a303b; border-radius:8px; margin-top:10px; padding-top:10px; }"
        "QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 4px; color:#8a92a0; }"
        "QPushButton { background:#4c9bff; color:white; border:none; border-radius:6px; padding:6px 14px; font-weight:600; }"
        "QPushButton:hover { background:#6aaeff; }"
        "QTableWidget { border:1px solid #2a303b; border-radius:6px; gridline-color:#2a303b; }"
        "QHeaderView::section { background:#1e222a; color:#8a92a0; border:none; padding:4px; }");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("NanoEX");
    applyDarkTheme(app);
    MainWindow w;
    w.show();
    return app.exec();
}
