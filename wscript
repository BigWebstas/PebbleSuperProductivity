#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

import os.path

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    # Link-time optimisation. The SDK already builds at -Os; adding -flto on top
    # cross-module inlines and dead-strips ~2.3 KB off each aggregate binary.
    # This matters because the emery build sits within ~60 bytes of PebbleOS's
    # hard 64 KB app virtual-size ceiling (PebbleProcessInfo.virtual_size is a
    # uint16_t) - without this, no further non-aplite C feature fits on emery.
    for e in ctx.all_envs.values():
        e.append_value('CFLAGS', ['-flto'])
        e.append_value('LINKFLAGS', ['-flto', '-Os'])

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/c/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})

    ctx.env = cached_env

    ctx.pbl_bundle(binaries=binaries,
        js=ctx.path.ant_glob('src/pkjs/**/*.js') + ctx.path.ant_glob('src/pkjs/**/*.json'),
        js_entry_file='src/pkjs/index.js')
