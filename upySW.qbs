import qbs

CppApplication {
    consoleApplication: true
    files: [
        "py/*.*",
        "lib/*/*.*",
        "extmod/*/*.*",
        "ports/swm320/*.*",
        "ports/swm320/*/*.*",
    ]
    cpp.includePaths: [
        ".",
        "./ports/swm320",
        "./lib/cmsis/inc"
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
