import qbs

CppApplication {
    consoleApplication: true
    files: [
        "py/*.*",
        "lib/*/*.*",
        "extmod/*/*.*",
        "ports/m480/*.*",
        "ports/m480/*/*.*",
    ]
    cpp.includePaths: [
        ".",
        "./ports/m480",
        "./ports/m480/chip/regs",
        "./lib/cmsis/inc"
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
