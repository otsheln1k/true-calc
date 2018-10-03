# -*- mode: python -*-
#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
import re
import os

top='.'
build='build'
srcdir='src/c'
test_file_prefix='test_'
remake_tests_cmd='cd \'{}\' \
&& make -qf Maketests \
|| make -f Maketests'.format(srcdir)

def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    """
    This method is used to configure your build. ctx.load(`pebble_sdk`)
    automatically configures a build for each valid platform in
    `targetPlatforms`. Platform-specific configuration: add your change
    after calling ctx.load('pebble_sdk') and make sure to set the correct
    environment first. Universal configuration: add your change prior to
    calling ctx.load('pebble_sdk').
    """
    # build and run tests before building; if either the build or any
    # test fails, don't build the app
    ctx.env['TESTS_OK'] = 0
    if os.system(remake_tests_cmd):
        print('building tests failed')
        return
    test_bin_regex = re.compile(r'^{}[^\.]+$'.format(test_file_prefix))
    for fn in (os.path.join(os.getcwd(), srcdir, fn)
               for fn in os.listdir(srcdir)
               if test_bin_regex.match(fn)):
        if os.system(fn):
            print('test failed: {}'.format(fn))
            return
    ctx.env.TESTS_OK = 1

    ctx.load('pebble_sdk')


def build(ctx):
    if not ctx.env.TESTS_OK:
        print('some tests failed; aborting')
        return

    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.env.CFLAGS.extend(['-D', '__PEBBLE__'])
        ctx.pbl_build(
            source=ctx.path.ant_glob(
                os.path.join(srcdir, '**/*.c'),
                excl=os.path.join(srcdir, '**/{}*'.format(test_file_prefix))),
            target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = \
                '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform,
                             'app_elf': app_elf,
                             'worker_elf': worker_elf})
            ctx.pbl_build(
                source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                target=worker_elf,
                bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env
    ctx.set_group("bundle")
    ctx.pbl_bundle(binaries=binaries)
