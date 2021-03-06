"""Adds repostories/archives."""

########################################################################
# DO NOT EDIT THIS FILE unless you are inside the
# https://github.com/3rdparty/stout-flags repository. If you
# encounter it anywhere else it is because it has been copied there in
# order to simplify adding transitive dependencies. If you want a
# different version of stout-flags follow the Bazel build
# instructions at https://github.com/3rdparty/stout-flags.
########################################################################

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def repos(external = True, repo_mapping = {}):
    maybe(
        http_archive,
        name = "com_google_protobuf",
        strip_prefix = "protobuf-3.19.1",
        urls = [
            "https://mirror.bazel.build/github.com/protocolbuffers/protobuf/archive/v3.19.1.tar.gz",
            "https://github.com/protocolbuffers/protobuf/archive/v3.19.1.tar.gz",
        ],
        sha256 = "87407cd28e7a9c95d9f61a098a53cf031109d451a7763e7dd1253abf8b4df422",
        repo_mapping = repo_mapping,
    )

    if external:
        maybe(
            git_repository,
            name = "com_github_3rdparty_stout_flags",
            remote = "https://github.com/3rdparty/stout-flags",
            commit = "67fbda639d346b8dfc5794eff4d82da28ecbedb7",
            shallow_since = "1651131019 +0000",
            repo_mapping = repo_mapping,
        )
