const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const dummy = b.addStaticLibrary(.{
        .name = "dummy",
        .target = target,
        .optimize = optimize,
    });
    dummy.linkLibC();
    dummy.addCSourceFiles(.{
        .files = &[_][]const u8{"dummy.c"},
        .flags = &[_][]const u8{"-std=c99"},
    });
    b.installArtifact(dummy);

    const ourolang = b.addExecutable(.{
        .name = "ourolang_repl",
        .target = target,
        .optimize = optimize,
    });
    ourolang.linkLibC();
    ourolang.linkLibCpp();
    ourolang.addCSourceFiles(.{
        .files = &[_][]const u8{
            "ouro_lang.cc",
        },
        .flags = &[_][]const u8{"-std=c++23"},
    });
    b.installArtifact(ourolang);

    const ourolang_cpp = b.addExecutable(.{
        .name = "ourolang_cpp",
        .target = target,
        .optimize = optimize,
    });
    ourolang_cpp.linkLibC();
    ourolang_cpp.linkLibCpp();
    ourolang_cpp.addCSourceFiles(.{
        .files = &[_][]const u8{"OuroLang_cpp/main.cc"},
        .flags = &[_][]const u8{"-std=c++23"},
    });
    b.installArtifact(ourolang_cpp);

    const ouroboros = b.addExecutable(.{
        .name = "ouroboros",
        .target = target,
        .optimize = optimize,
    });
    ouroboros.linkLibC();
    ouroboros.addCSourceFiles(.{
        .files = &[_][]const u8{
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/main.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/lexer.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/parser.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/ast.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/token.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/codegen.c",
        },
        .flags = &[_][]const u8{"-std=c11"},
    });
    ouroboros.addIncludePath(.{ .cwd_relative = "Ouroboros/Ouroboros_Compiler/include" });
    b.installArtifact(ouroboros);

    const ouro_lang = b.addExecutable(.{
        .name = "ouro_lang",
        .target = target,
        .optimize = optimize,
    });
    ouro_lang.linkLibCpp();
    const src_files = &[_][]const u8{
        "src/main.cc",
        "src/repl_binding.cc",
        "src/codegen.cc",
        "src/core.cc",
        "src/core.impl.cc",
        "src/ir.cc",
        "src/parser.cc",
        "src/parser.impl.cc",
        "src/ir/partitions/passes.cc",
        "src/ir/partitions/transforms.cc",
    };
    ouro_lang.addIncludePath(.{ .cwd_relative = "include" });
    ouro_lang.addCSourceFiles(.{ .files = src_files, .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" } });
    b.installArtifact(ouro_lang);

    const ouro_mod = b.addExecutable(.{
        .name = "ouro_mod",
        .target = target,
        .optimize = optimize,
    });
    ouro_mod.linkLibCpp();
    ouro_mod.addCSourceFiles(.{
        .files = &[_][]const u8{
            "ouro_mod/src/main.cc",
            "ouro_mod/src/foundation/lexer.cc",
            "ouro_mod/src/essentials/environment.cc",
        },
        .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" },
    });
    const mod_modules = &[_][]const u8{
        "ouro_mod/src/foundation/lexer.cppm",
        "ouro_mod/src/essentials/environment.cppm",
    };
    ouro_mod.addCSourceFiles(.{ .files = mod_modules, .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" } });
    b.installArtifact(ouro_mod);

    const ouro_mod_run = b.addRunArtifact(ouro_mod);
    b.step("mod-run", "Run modular example").dependOn(&ouro_mod_run.step);

    const mod_tests = b.addExecutable(.{
        .name = "ouro_mod_tests",
        .target = target,
        .optimize = optimize,
    });
    mod_tests.linkLibCpp();
    mod_tests.addCSourceFiles(.{
        .files = &[_][]const u8{
            "ouro_mod/tests/foundation_tests.cc",
            "ouro_mod/src/foundation/lexer.cc",
            "ouro_mod/src/essentials/environment.cc",
        },
        .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" },
    });
    mod_tests.addCSourceFiles(.{ .files = mod_modules, .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" } });
    const test_cmd = b.addRunArtifact(mod_tests);
    b.step("mod-test", "Run module tests").dependOn(&test_cmd.step);

    const lexer_tests = b.addExecutable(.{
        .name = "lexer_tests",
        .target = target,
        .optimize = optimize,
    });
    lexer_tests.linkLibCpp();
    lexer_tests.addIncludePath(.{ .cwd_relative = "include" });
    lexer_tests.addCSourceFiles(.{ .files = src_files, .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" } });
    lexer_tests.addCSourceFiles(.{ .files = &[_][]const u8{ "tests/lexer_tests.cc" }, .flags = &[_][]const u8{ "-std=c++23" } });
    const run_lexer = b.addRunArtifact(lexer_tests);

    const codegen_tests = b.addExecutable(.{
        .name = "codegen_tests",
        .target = target,
        .optimize = optimize,
    });
    codegen_tests.linkLibCpp();
    codegen_tests.addIncludePath(.{ .cwd_relative = "include" });
    codegen_tests.addCSourceFiles(.{ .files = src_files, .flags = &[_][]const u8{ "-std=c++23", "-fmodules-ts" } });
    codegen_tests.addCSourceFiles(.{ .files = &[_][]const u8{ "tests/codegen_tests.cc" }, .flags = &[_][]const u8{ "-std=c++23" } });
    const run_codegen = b.addRunArtifact(codegen_tests);

    const all_tests = b.step("test", "Run C++ unit tests");
    all_tests.dependOn(&run_lexer.step);
    all_tests.dependOn(&run_codegen.step);
}
