set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(doom_firmware C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(doom_firmware)

target_sources(doom_firmware PRIVATE
    main.c
    lib/button/bootsel_button.c
)

add_subdirectory(lib/config)
add_subdirectory(lib/pwr_button)
add_subdirectory(lib/touch)
add_subdirectory(lib/qspi_pio)
add_subdirectory(lib/amoled)
add_subdirectory(lib/gui)
add_subdirectory(lib/fonts)
add_subdirectory(lib/display)

include_directories(./lib/config)
include_directories(./lib/button)
include_directories(./lib/pwr_button)
include_directories(./lib/touch)
include_directories(./lib/qspi_pio)
include_directories(./lib/amoled)
include_directories(./lib/gui)
include_directories(./lib/fonts)
include_directories(./lib/display)

target_link_libraries(doom_firmware PRIVATE
    config
    pwr_button
    touch
    display
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_dma
    m
)

pico_enable_stdio_usb(doom_firmware 1)
pico_enable_stdio_uart(doom_firmware 1)

pico_add_extra_outputs(doom_firmware)

# ---------------------------------------------------------------------------
# doom: the actual game, adapted from kilograham/rp2040-doom (vendored under
# engine/ - see engine/README or doom/docs/DECISIONS.md for what's upstream
# vs. ours). Separate executable target from the calibration firmware above.
# ---------------------------------------------------------------------------
add_subdirectory(lib/audio_pio)
add_subdirectory(lib/es8311)
add_subdirectory(lib/bootlog)

add_executable(doom)

file(GLOB DOOM_ENGINE_ROOT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/*.c)
file(GLOB DOOM_GAME_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/doom/*.c)
file(GLOB DOOM_PICO_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/pico/*.c)

target_sources(doom PRIVATE
    ${DOOM_ENGINE_ROOT_SOURCES}
    ${DOOM_GAME_SOURCES}
    ${DOOM_PICO_SOURCES}
    engine/pico/blit.S
    engine/pd_render.cpp
    engine/adpcm-xq/adpcm-lib.c
)

target_include_directories(doom PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/engine
    ${CMAKE_CURRENT_LIST_DIR}/engine/doom
    ${CMAKE_CURRENT_LIST_DIR}/engine/pico
    ${CMAKE_CURRENT_LIST_DIR}/engine/adpcm-xq
)

target_link_libraries(doom PRIVATE
    config
    amoled
    gui
    fonts
    bootlog
    qspi_pio
    audio_pio
    es8311
    touch
    pwr_button
    pico_stdlib
    pico_multicore
    hardware_i2c
    hardware_pio
    hardware_dma
    hardware_gpio
    hardware_interp
    m
)

target_compile_definitions(doom PRIVATE
    PICODOOM_RENDER_NEWHOPE=1
    MERGE_DISTSCALE0_INTO_VIEWCOSSINANGLE=1

    DOOM_TINY=1
    SHRINK_MOBJ=1
    NO_RDRAW=1

    DOOM_ONLY=1
    DOOM_SMALL=1
    DOOM_CONST=1

    SOUND_LOW_PASS=1
    NUM_SOUND_CHANNELS=8

    NO_USE_CHECKSUM=1
    NO_USE_RELOAD=1
    USE_SINGLE_IWAD=1
    NO_USE_WIPE=1
    NO_USE_JOYSTICK=1
    NO_USE_DEH=1
    NO_USE_MUSIC_PACKS=1
    NO_USE_ENDDOOM=1
    NO_USE_MOUSE=1

    USE_FLAT_MAX_256=1
    USE_MEMMAP_ONLY=1
    USE_LIGHTMAP_INDEXES=1

    USE_ERASE_FRAME=1
    NO_DRAW_MID=1
    NO_DRAW_TOP=1
    NO_DRAW_BOTTOM=1
    NO_DRAW_MASKED=1
    NO_DRAW_SKY=1
    NO_DRAW_SPRITES=1
    NO_DRAW_PSPRITES=1

    NO_VISPLANE_GUTS=1
    NO_VISPLANE_CACHES=1
    NO_DRAWSEGS=1
    NO_VISSPRITES=1
    NO_MASKED_FLOOR_CLIP=1

    PD_DRAW_COLUMNS=1
    PD_DRAW_MARKERS=1
    PD_DRAW_PLANES=1
    PD_SCALE_SORT=1
    PD_CLIP_WALLS=1
    PD_QUANTIZE=1
    PD_SANITY=1
    PD_COLUMNS=1
    PICO_DOOM=1

    NO_USE_DS_COLORMAP=1
    NO_USE_DC_COLORMAP=1
    USE_READONLY_MMAP=1

    NO_USE_TIMIDITY=1
    NO_USE_GUS=1
    NO_USE_LIBSAMPLERATE=1

    NO_USE_STATE_MISC=1

    USE_RAW_MAPNODE=1
    USE_RAW_MAPVERTEX=1
    USE_RAW_MAPSEG=1
    USE_RAW_MAPLINEDEF=1
    USE_RAW_MAPTHING=1

    USE_INDEX_LINEBUFFER=1
    NO_USE_ZLIGHT=1
    NO_Z_ZONE_ID=1

    Z_MALOOC_EXTRA_DATA=1
    USE_THINKER_POOL=1
    NO_INTERCEPTS_OVERRUN=1

    TEMP_IMMUTABLE_DISABLED=1
    USE_CONST_SFX=1
    USE_CONST_MUSIC=1

    NO_DEMO_RECORDING=1
    PICO_NO_TIMING_DEMO=1
    NO_USE_EXIT=1

    PICO_DEBUG_PIN_BASE=18
    PICO_DEBUG_PIN_COUNT=2

    NO_USE_NET=1
    USE_PICO_NET=1

    NO_FILE_ACCESS=1
    SAVE_COMPRESSED=1
    LOAD_COMPRESSED=1

    NO_USE_ARGS=1
    NO_USE_SAVE_CONFIG=1
    NO_USE_FLOAT=1

    USE_VANILLA_KEYBOARD_MAPPING_ONLY=1
    NO_USE_LOADING_DISK=1

    USE_WHD=1
    # whd_gen (as we built it) always emits the "super tiny" WHD format
    # ("IWHX" magic) regardless of intent - matching the firmware's WAD
    # magic check to that instead of chasing a different whd_gen mode.
    # DEMO1_ONLY is part of the same upstream "tiny_settings" bundle and is
    # fine for now (doom1.wad, shareware, is single-episode anyway) - if/
    # when swapping in the multi-episode retail WAD, whd_gen and this need
    # revisiting together. See doom/docs/DECISIONS.md.
    WHD_SUPER_TINY=1
    DEMO1_ONLY=1
    NO_USE_FINALE_CAST=1
    NO_USE_FINALE_BUNNY=1
    NO_Z_MALLOC_USER_PTR=1

    FIXED_SCREENWIDTH=1
    FLOOR_CEILING_CLIP_8BIT=1

    USE_MUSX=1
    MUSX_COMPRESSED=1
    USE_DIRECT_MIDI_LUMP=1

    NO_SCREENSHOT=1
    NO_USE_BOUND_CONFIG=1
    USE_FPS=1

    USE_MEMORY_WAD=1
    PICO_DEBUG_MALLOC=1

    # Was 0x4f8 (1272 bytes, upstream's RP2040 value) - way too tight now
    # that core1 actually renders real content (menu fonts/vpatches, not
    # just the simple title splash) through pd_render.cpp's deeper call
    # paths (flush_visplanes alone has ~1.8KB of locals). Suspected cause
    # of a hard-to-pin-down freeze + memory-corruption symptoms (bootlog
    # text appearing in screen regions its own code never targets) first
    # seen 2026-08-16 navigating the menu - see doom/docs/DECISIONS.md.
    # NOT free to size arbitrarily from the ~55KB general RAM margin: this
    # lives in SCRATCH_X, a separate, fixed 4KB hardware SRAM bank on
    # RP2350 (see default_locations.ld) dedicated to core1's stack - 0x1000
    # is the most this can ever be. An 0x2000 attempt overflowed into
    # SCRATCH_Y, which core0's own default stack needs.
    PICO_CORE1_STACK_SIZE=0x1000
    NO_IERROR=1

    NO_ZONE_DEBUG=1

    # 16MB flash gives us lots of headroom vs upstream's cramped RP2040
    # boards - pick an offset with generous margin past any plausible
    # firmware size instead of upstream's tight 0x10048000.
    TINY_WAD_ADDR=0x10200000

    # TEMPORARY (see doom/docs/DECISIONS.md 2026-08-16): skips both
    # S_StartSound() (sound effects - our audio_pio driver blocks the
    # caller for real, upstream's doesn't, a real architectural gap not
    # yet fixed) and S_ChangeMusic() (level music - a named lump lookup
    # that likely doesn't exist for our MUSX_COMPRESSED WAD, hitting an
    # I_Error()->bkpt hard fault via NO_IERROR=1, also not yet fixed).
    # Remove this line to re-enable sound/music once either is resolved.
    DEBUG_NO_SOUND=1
)
# Deliberately NOT set (unlike upstream's RP2040-cramped build):
# PICO_TIME_DEFAULT_ALARM_POOL_DISABLED / _MAX_TIMERS, PICO_DISABLE_SHARED_IRQ_HANDLERS,
# PICO_USE_OPTIMISTIC_SBRK, PICO_USE_SW_SPIN_LOCKS=0, PICO_USE_STACK_GUARDS=0 - these
# change how SDK-internal sources like pico_time.c compile, and since they were only
# PRIVATE to this target while lib/config (and everything it pulls in) compiled without
# them, the two differently-configured copies of the same SDK source collided at link
# time ("multiple definition of sleep_us" etc). RP2350's 520KB SRAM (vs RP2040's 264KB,
# which is what these macros exist to claw back) means we don't need this savings badly
# enough to fight SDK-wide compile-definition consistency for it.
#
# PICO_HEAP_SIZE=0 / USE_ZONE_FOR_MALLOC=1 similarly dropped: they make i_system.c define
# its own __wrap_malloc/calloc/free (routing stray malloc() calls through Doom's Z_zone
# allocator to save RAM), which collides with pico_malloc's own __wrap_* - the newer SDK
# links pico_malloc unconditionally, upstream's SDK apparently didn't. Doom's actual
# allocations go through Z_Malloc regardless: this only affects incidental malloc() calls
# from vendored library code (lodepng etc), which can just use the normal newlib heap.

target_compile_options(doom PRIVATE
    -Wno-format-truncation
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-unused-variable
    -fno-common
    -fdata-sections
)
target_link_options(doom PRIVATE -Wl,--sort-section=alignment)

# NOT set to "none" (unlike upstream, which can get away with it since
# Doom's own code is fully float-free via NO_USE_FLOAT=1): mp3player's
# audio_pio.c (lib/audio_pio/audio_pio.c, set_mclk_frequency) does real
# double-precision math, and "none" replaces float/double ops with stubs
# that hang rather than compute anything - this was the actual cause of a
# hang inside mclk_pio_init() during ES8311 setup. The earlier appearance
# of a second, unrelated regression (AMOLED_1IN8_DisplayWindows() being
# intermittently unreliable) briefly looked connected to this but wasn't -
# see doom/docs/DECISIONS.md.

pico_wrap_function(doom malloc)
pico_wrap_function(doom calloc)
pico_wrap_function(doom free)

# USB stdio needs the default alarm pool - we no longer disable that (see the
# note above on dropped RAM-saving defines), so USB is back on for debug output.
pico_enable_stdio_usb(doom 1)
pico_enable_stdio_uart(doom 1)

pico_add_extra_outputs(doom)
