add_rules("mode.debug", "mode.release")

target("gobang-client")
    add_rules("qt.widgetapp")
    add_headerfiles(
        "*.h",
        "base/*.h",
        "local/*.h",
        "online/*.h",
        "robot/*.h",
        "lib/openjson/*.h"
    )

    add_files(
        "*.cpp",
        "base/*.cpp",
        "local/*.cpp",
        "online/*.cpp",
        "robot/*.cpp",
        "lib/openjson/*.cpp"
    )
    add_files(
        "mainwindow.ui",
        "local/game.ui",
        "local/home.ui",
        "local/settingdialog.ui",
        "online/lobby.ui",
        "online/onlinegame.ui"
    )
    add_files(
        "images.qrc",
        "app_win32.rc"
    )
    -- add files with Q_OBJECT meta (only for qt.moc)!!!

    add_files(
        "mainwindow.h",
        "local/game.h",
        "local/home.h",
        "local/settingdialog.h",
        "online/lobby.h",
        "online/onlinegame.h",
        "online/recvthread.h",
        "local/robotthread.h"

    )
    add_includedirs(
        ".",
        "base",
        "local",
        "online",
        "robot",
        "lib/openjson"
    )

    add_defines(
        "WIN32"
    )
