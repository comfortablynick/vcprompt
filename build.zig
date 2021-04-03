const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();
    const static = b.option(bool, "static", "build static library instead of shared") orelse false;

    const cflags = [_][]const u8{
        "-std=c99",
        // "-pedantic",
        // "-Werror",
        // "-Wall",
    };

    const source_files = [_][]const u8{
        "src/vcprompt.c",
        // "src/svn.h",
        "src/svn.c",
        // "src/hg.h",
        "src/hg.c",
        // "src/git.h",
        "src/git.c",
        // "src/fossil.h",
        "src/fossil.c",
        // "src/cvs.h",
        "src/cvs.c",
        // "src/common.h",
        "src/common.c",
        // "src/capture.h",
        "src/capture.c",
    };
    const exe = b.addExecutable("vctest", null);
    exe.setBuildMode(mode);
    b.setInstallPrefix("$HOME/.local/bin");

    for (source_files) |source| {
        exe.addCSourceFile(source, cflags);
    }
    exe.addIncludeDir("src");
    exe.linkSystemLibrary("c");
    b.default_step.dependOn(&exe.step);

    // examples

    // const token_list_exe = b.addExecutable("token_list", null);
    // token_list_exe.setBuildMode(mode);
    // token_list_exe.addCSourceFile("example/token_list.c", example_cflags);
    // token_list_exe.linkLibrary(lib);
    // token_list_exe.addIncludeDir("include");

    // b.default_step.dependOn(&token_list_exe.step);

    // test

    // const primitives_test_exe = b.addExecutable("primitives_test", null);
    // primitives_test_exe.setBuildMode(mode);
    // primitives_test_exe.addCSourceFile("test/primitives.c", example_cflags);
    // primitives_test_exe.addIncludeDir("include");
    // primitives_test_exe.linkLibrary(lib);

    // const run_test_cmd = primitives_test_exe.run();

    // const test_step = b.step("test", "Run the tests");
    // test_step.dependOn(&run_test_cmd.step);

    // b.installFile("include/laxjson.h", "include/laxjson.h");
}
