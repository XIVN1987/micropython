import qbs

CppApplication {
    consoleApplication: true
    files: [
        "py/*.*",
        "lib/*/*.*",
        "extmod/*/*.*",
        "ports/mt7687/*.*",
        "ports/mt7687/*/*.*",
    ]
    cpp.includePaths: [
        ".",
        "./ports/mt7687",
        "./lib/cmsis/inc"
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
