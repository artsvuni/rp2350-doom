set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(doom_firmware C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# The fixed-memory MUSX synthesizer is part of the handheld firmware so music
# can be enabled from Doom's normal Options menu. The runtime volume starts at
# zero on-device; sound effects remain audible by default.
option(DOOM_ENABLE_MUSIC "Enable fixed-memory MUSX music synthesis" ON)
option(DOOM_MUSIC_SPEAKER_MASTERING
    "Use smoother synthesis and music-only small-speaker mastering" OFF)
option(DOOM_MUSIC_SMOOTH_SYNTH
    "Use low-cost smooth voices and note-edge ramps without mastering" OFF)
set(DOOM_MUSIC_PERCUSSION_PERCENT "50" CACHE STRING
    "MIDI percussion-channel gain percentage (0 to 100)")
option(DOOM_MUSIC_LAB
    "Boot directly into the standalone E1M1 music listening lab" OFF)
set(DOOM_MUSIC_LAB_VARIANT "0" CACHE STRING
    "Music lab profile: 0 baseline, 1 smooth synth, 2 22.05kHz, 3 codec DRC")
set_property(CACHE DOOM_MUSIC_LAB_VARIANT PROPERTY STRINGS 0 1 2 3)
option(DOOM_HANDHELD_AUTO_SAVE_NAMES
    "Save immediately with generated slot/map/time names instead of typing" OFF)
option(DOOM_FLASH_SAFE_SAVES
    "Coordinate both RP2350 cores while erasing/programming save flash" OFF)
option(DOOM_ENABLE_PROFILING "Capture a bounded reset-persistent performance report" OFF)
option(DOOM_ASYNC_AMOLED "Overlap AMOLED DMA with presentation packing" OFF)
option(DOOM_HYBRID_CONTROLS
    "Use two-axis touch movement/turning and tap PWR fire" OFF)
option(DOOM_TOUCH_DPAD
    "Use a fixed eight-way bottom-left D-pad instead of floating touch" OFF)
option(DOOM_TOUCH_DPAD_THUMB_ZONES
    "Use asymmetric thumb zones instead of the radial D-pad" OFF)
option(DOOM_TOUCH_DPAD_OVERLAY
    "Draw temporary D-pad outlines and active-zone highlight" OFF)
option(DOOM_BOOT_NEXT_WEAPON
    "Safety-gated BOOT release cycles to Doom's next weapon" OFF)
option(DOOM_BOOT_HOLD_STRAFE
    "Holding BOOT makes thumb-zone left/right strafe" OFF)
option(DOOM_BOOT_WITH_SOUND_EFFECTS
    "Gate C only: retain sound effects while BOOT next-weapon is enabled" OFF)
option(DOOM_ROLL_STRAFE
    "Add experimental accelerometer roll strafing to hybrid controls" OFF)
set(DOOM_PROFILE_CAPTURE_SECONDS "60" CACHE STRING
    "Profiler capture duration in seconds")
set(DOOM_DISPLAY_WIDTH "320" CACHE STRING
    "Landscape Doom output width: 320, 384, 416, or 448")
set_property(CACHE DOOM_DISPLAY_WIDTH PROPERTY STRINGS 320 384 416 448)
set(DOOM_DISPLAY_HEIGHT_OVERRIDE "AUTO" CACHE STRING
    "Optional 448-wide output height: AUTO, 280, 320, 336, or 368")
set_property(CACHE DOOM_DISPLAY_HEIGHT_OVERRIDE PROPERTY STRINGS
    AUTO 280 320 336 368)
set(DOOM_PANEL_CHUNK_ROWS "40" CACHE STRING
    "Transpose tile height: AUTO, 8, 10, 20, or 40")
set_property(CACHE DOOM_PANEL_CHUNK_ROWS PROPERTY STRINGS AUTO 8 10 20 40)

if(NOT DOOM_MUSIC_LAB_VARIANT MATCHES "^[0-3]$")
    message(FATAL_ERROR "DOOM_MUSIC_LAB_VARIANT must be 0, 1, 2, or 3")
endif()

if(NOT DOOM_MUSIC_PERCUSSION_PERCENT MATCHES "^[0-9]+$"
   OR DOOM_MUSIC_PERCUSSION_PERCENT GREATER 100)
    message(FATAL_ERROR "DOOM_MUSIC_PERCUSSION_PERCENT must be 0 to 100")
endif()

if(DOOM_MUSIC_LAB AND NOT DOOM_ENABLE_MUSIC)
    message(FATAL_ERROR "DOOM_MUSIC_LAB requires DOOM_ENABLE_MUSIC=ON")
endif()

if(DOOM_MUSIC_LAB AND DOOM_MUSIC_SPEAKER_MASTERING)
    message(FATAL_ERROR
        "Music-lab profiles use the accepted lightweight path; configure "
        "DOOM_MUSIC_SPEAKER_MASTERING=OFF")
endif()

if(DOOM_MUSIC_LAB AND DOOM_MUSIC_LAB_VARIANT STREQUAL "2")
    set(DOOM_AUDIO_SAMPLE_RATE 22050)
    set(DOOM_AUDIO_BUFFER_SAMPLES 256)
else()
    set(DOOM_AUDIO_SAMPLE_RATE 44100)
    set(DOOM_AUDIO_BUFFER_SAMPLES 512)
endif()

if(DOOM_DISPLAY_WIDTH STREQUAL "320")
    set(DOOM_DISPLAY_HEIGHT 200)
elseif(DOOM_DISPLAY_WIDTH STREQUAL "384")
    set(DOOM_DISPLAY_HEIGHT 240)
elseif(DOOM_DISPLAY_WIDTH STREQUAL "416")
    set(DOOM_DISPLAY_HEIGHT 260)
elseif(DOOM_DISPLAY_WIDTH STREQUAL "448")
    set(DOOM_DISPLAY_HEIGHT 280)
else()
    message(FATAL_ERROR
        "DOOM_DISPLAY_WIDTH must be one of 320, 384, 416, or 448")
endif()

if(NOT DOOM_DISPLAY_HEIGHT_OVERRIDE STREQUAL "AUTO")
    if(NOT DOOM_DISPLAY_WIDTH STREQUAL "448")
        message(FATAL_ERROR
            "DOOM_DISPLAY_HEIGHT_OVERRIDE is only supported at 448px width")
    endif()
    if(NOT DOOM_DISPLAY_HEIGHT_OVERRIDE MATCHES "^(280|320|336|368)$")
        message(FATAL_ERROR
            "DOOM_DISPLAY_HEIGHT_OVERRIDE must be AUTO, 280, 320, 336, or 368")
    endif()
    set(DOOM_DISPLAY_HEIGHT ${DOOM_DISPLAY_HEIGHT_OVERRIDE})
endif()

if(DOOM_PANEL_CHUNK_ROWS STREQUAL "AUTO")
    math(EXPR DOOM_PANEL_CHUNK_REMAINDER "${DOOM_DISPLAY_HEIGHT} % 8")
    if(DOOM_PANEL_CHUNK_REMAINDER EQUAL 0)
        set(DOOM_PANEL_CHUNK_ROWS_RESOLVED 8)
    else()
        # 416x260 is the only current mode not divisible by eight.
        set(DOOM_PANEL_CHUNK_ROWS_RESOLVED 10)
    endif()
elseif(DOOM_PANEL_CHUNK_ROWS MATCHES "^(8|10|20|40)$")
    set(DOOM_PANEL_CHUNK_ROWS_RESOLVED ${DOOM_PANEL_CHUNK_ROWS})
else()
    message(FATAL_ERROR
        "DOOM_PANEL_CHUNK_ROWS must be AUTO, 8, 10, 20, or 40")
endif()

math(EXPR DOOM_PANEL_CHUNK_REMAINDER
    "${DOOM_DISPLAY_HEIGHT} % ${DOOM_PANEL_CHUNK_ROWS_RESOLVED}")
if(NOT DOOM_PANEL_CHUNK_REMAINDER EQUAL 0)
    if(DOOM_DISPLAY_HEIGHT EQUAL 336 AND
       DOOM_PANEL_CHUNK_ROWS_RESOLVED EQUAL 20)
        message(STATUS
            "448x336 uses a padded final 16-row image tile in one 20-row "
            "panel transaction")
    elseif(DOOM_DISPLAY_HEIGHT EQUAL 368 AND
           DOOM_PANEL_CHUNK_ROWS_RESOLVED EQUAL 20 AND
           DOOM_ASYNC_AMOLED)
        message(STATUS
            "448x368 uses a final overlapping 20-row panel transaction "
            "assembled from the existing asynchronous buffers")
    else()
        message(FATAL_ERROR
            "${DOOM_DISPLAY_HEIGHT} output rows must be divisible by "
            "DOOM_PANEL_CHUNK_ROWS=${DOOM_PANEL_CHUNK_ROWS_RESOLVED}; only "
            "the bounded 448x336/20-row padded case and asynchronous "
            "448x368/20-row overlapping case are supported")
    endif()
endif()

if(DOOM_ASYNC_AMOLED AND NOT DOOM_PANEL_CHUNK_ROWS_RESOLVED EQUAL 20)
    message(FATAL_ERROR
        "DOOM_ASYNC_AMOLED requires DOOM_PANEL_CHUNK_ROWS=20 so two buffers "
        "use the same SRAM as the proven single 40-row buffer")
endif()

if(DOOM_TOUCH_DPAD AND NOT DOOM_HYBRID_CONTROLS)
    message(FATAL_ERROR
        "DOOM_TOUCH_DPAD requires DOOM_HYBRID_CONTROLS=ON")
endif()

if(DOOM_TOUCH_DPAD_THUMB_ZONES AND NOT DOOM_TOUCH_DPAD)
    message(FATAL_ERROR
        "DOOM_TOUCH_DPAD_THUMB_ZONES requires DOOM_TOUCH_DPAD=ON")
endif()

if(DOOM_TOUCH_DPAD_OVERLAY AND NOT DOOM_TOUCH_DPAD_THUMB_ZONES)
    message(FATAL_ERROR
        "DOOM_TOUCH_DPAD_OVERLAY requires DOOM_TOUCH_DPAD_THUMB_ZONES=ON")
endif()

if(DOOM_TOUCH_DPAD_OVERLAY AND NOT DOOM_DISPLAY_WIDTH STREQUAL "448")
    message(FATAL_ERROR
        "DOOM_TOUCH_DPAD_OVERLAY currently targets the 448px full-width view")
endif()

if(DOOM_BOOT_NEXT_WEAPON AND NOT DOOM_HYBRID_CONTROLS)
    message(FATAL_ERROR
        "DOOM_BOOT_NEXT_WEAPON is a Gate B extension of the hybrid controls")
endif()

if(DOOM_BOOT_HOLD_STRAFE AND NOT DOOM_BOOT_NEXT_WEAPON)
    message(FATAL_ERROR
        "DOOM_BOOT_HOLD_STRAFE requires DOOM_BOOT_NEXT_WEAPON=ON")
endif()

if(DOOM_BOOT_HOLD_STRAFE AND NOT DOOM_TOUCH_DPAD_THUMB_ZONES)
    message(FATAL_ERROR
        "DOOM_BOOT_HOLD_STRAFE currently requires the F14 thumb zones")
endif()

if(DOOM_BOOT_WITH_SOUND_EFFECTS AND NOT DOOM_BOOT_NEXT_WEAPON)
    message(FATAL_ERROR
        "DOOM_BOOT_WITH_SOUND_EFFECTS requires DOOM_BOOT_NEXT_WEAPON=ON")
endif()

if(NOT DOOM_PROFILE_CAPTURE_SECONDS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "DOOM_PROFILE_CAPTURE_SECONDS must be a positive integer")
endif()

message(STATUS
    "Doom AMOLED output: ${DOOM_DISPLAY_WIDTH}x${DOOM_DISPLAY_HEIGHT}; "
    "tile=${DOOM_PANEL_CHUNK_ROWS_RESOLVED}; async=${DOOM_ASYNC_AMOLED}; "
    "profiling=${DOOM_ENABLE_PROFILING}; dpad=${DOOM_TOUCH_DPAD}; "
    "auto_save_names=${DOOM_HANDHELD_AUTO_SAVE_NAMES}; "
    "flash_safe_saves=${DOOM_FLASH_SAFE_SAVES}; "
    "music_percussion=${DOOM_MUSIC_PERCUSSION_PERCENT}%; "
    "music_smooth=${DOOM_MUSIC_SMOOTH_SYNTH}; "
    "music_mastering=${DOOM_MUSIC_SPEAKER_MASTERING}; "
    "music_lab=${DOOM_MUSIC_LAB}; lab_variant=${DOOM_MUSIC_LAB_VARIANT}; "
    "audio=${DOOM_AUDIO_SAMPLE_RATE}Hz/${DOOM_AUDIO_BUFFER_SAMPLES}; "
    "thumb_zones=${DOOM_TOUCH_DPAD_THUMB_ZONES}; "
    "dpad_overlay=${DOOM_TOUCH_DPAD_OVERLAY}; "
    "boot_next_weapon=${DOOM_BOOT_NEXT_WEAPON}; "
    "boot_hold_strafe=${DOOM_BOOT_HOLD_STRAFE}; "
    "boot_with_sfx=${DOOM_BOOT_WITH_SOUND_EFFECTS}")

pico_sdk_init()

add_executable(doom_firmware)

target_sources(doom_firmware PRIVATE
    main.c
    lib/button/bootsel_button.c
)

add_subdirectory(lib/config)
add_subdirectory(lib/pwr_button)
add_subdirectory(lib/touch)
add_subdirectory(lib/qmi8658)
add_subdirectory(lib/qspi_pio)
add_subdirectory(lib/amoled)
add_subdirectory(lib/gui)
add_subdirectory(lib/fonts)
add_subdirectory(lib/display)

if(DOOM_ENABLE_PROFILING)
    target_compile_definitions(amoled PRIVATE AMOLED_ENABLE_PROFILING=1)
endif()

include_directories(./lib/config)
include_directories(./lib/button)
include_directories(./lib/pwr_button)
include_directories(./lib/touch)
include_directories(./lib/qmi8658)
include_directories(./lib/qspi_pio)
include_directories(./lib/amoled)
include_directories(./lib/gui)
include_directories(./lib/fonts)
include_directories(./lib/display)

target_link_libraries(doom_firmware PRIVATE
    config
    pwr_button
    touch
    qmi8658
    display
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_dma
    pico_flash
    m
)

pico_enable_stdio_usb(doom_firmware 1)
pico_enable_stdio_uart(doom_firmware 1)

pico_add_extra_outputs(doom_firmware)

# ---------------------------------------------------------------------------
# boot_safety_probe: deliberately isolated first gate for runtime BOOT input.
# It has no display, codec, amplifier, DMA, I2C, or multicore dependencies.
# A confirmed short BOOT press/release reboots into ROM BOOTSEL so the host can
# verify detection without trusting USB serial output. Do not replace this
# target with a BOOT-reading Doom build until the physical probe has passed.
# ---------------------------------------------------------------------------
add_executable(boot_safety_probe)

target_sources(boot_safety_probe PRIVATE
    boot_safety_probe.c
    lib/button/bootsel_button.c
)

target_include_directories(boot_safety_probe PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lib/button
)

target_link_libraries(boot_safety_probe PRIVATE
    pico_stdlib
    pico_flash
    hardware_gpio
)

target_compile_definitions(boot_safety_probe PRIVATE
    # Return an error instead of asserting if the SDK cannot establish its
    # flash-safe context. The probe skips that sample rather than guessing it
    # was a button press.
    PICO_FLASH_ASSERT_ON_UNSAFE=0
)

pico_enable_stdio_usb(boot_safety_probe 0)
pico_enable_stdio_uart(boot_safety_probe 0)
pico_add_extra_outputs(boot_safety_probe)

# ---------------------------------------------------------------------------
# doom: the actual game, adapted from kilograham/rp2040-doom (vendored under
# engine/ - see engine/README or doom/docs/DECISIONS.md for what's upstream
# vs. ours). Separate executable target from the calibration firmware above.
# ---------------------------------------------------------------------------
add_subdirectory(lib/audio_pio)
add_subdirectory(lib/es8311)
add_subdirectory(lib/bootlog)

target_compile_definitions(audio_pio PRIVATE
    DOOM_AUDIO_BUFFER_SAMPLES=${DOOM_AUDIO_BUFFER_SAMPLES})
target_compile_definitions(es8311 PRIVATE
    DOOM_MUSIC_LAB=$<BOOL:${DOOM_MUSIC_LAB}>
    DOOM_MUSIC_LAB_VARIANT=${DOOM_MUSIC_LAB_VARIANT})

if(DOOM_BOOT_NEXT_WEAPON OR DOOM_FLASH_SAFE_SAVES)
    # No DMA-visible source may live in XIP while BOOT sampling or a save write
    # temporarily makes flash unavailable. Gate B additionally disables audio
    # unless Gate C is explicitly ON.
    target_compile_definitions(audio_pio PRIVATE DOOM_BOOT_FLASH_SAFE_DMA=1)
endif()

if(DOOM_BOOT_NEXT_WEAPON)
    if(NOT DOOM_BOOT_WITH_SOUND_EFFECTS)
        target_compile_definitions(config PRIVATE DOOM_BOOT_GATE_AUDIO_DISABLED=1)
    endif()
endif()

add_executable(doom)

file(GLOB DOOM_ENGINE_ROOT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/*.c)
file(GLOB DOOM_GAME_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/doom/*.c)
file(GLOB DOOM_PICO_SOURCES ${CMAKE_CURRENT_LIST_DIR}/engine/pico/*.c)

# These files are referenced only by the optional embedded music backend.
# Excluding them from the default build also drops the static MUSX iterator,
# recovering its zone-adjacent SRAM rather than merely compiling a no-op UI.
if(NOT DOOM_ENABLE_MUSIC)
    list(FILTER DOOM_ENGINE_ROOT_SOURCES EXCLUDE REGEX "/(midifile|musx_decoder|mus2mid)\\.c$")
endif()

target_sources(doom PRIVATE
    ${DOOM_ENGINE_ROOT_SOURCES}
    ${DOOM_GAME_SOURCES}
    ${DOOM_PICO_SOURCES}
    engine/pico/blit.S
    engine/pd_render.cpp
    engine/adpcm-xq/adpcm-lib.c
)

if(DOOM_BOOT_NEXT_WEAPON)
    target_sources(doom PRIVATE lib/button/bootsel_button.c)
endif()

target_include_directories(doom PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/engine
    ${CMAKE_CURRENT_LIST_DIR}/engine/doom
    ${CMAKE_CURRENT_LIST_DIR}/engine/pico
    ${CMAKE_CURRENT_LIST_DIR}/engine/adpcm-xq
)

if(DOOM_BOOT_NEXT_WEAPON)
    target_include_directories(doom PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/button)
endif()

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
    qmi8658
    pico_stdlib
    pico_multicore
    hardware_i2c
    hardware_pio
    hardware_dma
    hardware_gpio
    hardware_interp
    m
)

if(DOOM_BOOT_NEXT_WEAPON OR DOOM_FLASH_SAFE_SAVES)
    target_link_libraries(doom PRIVATE pico_flash)
endif()

target_compile_definitions(doom PRIVATE
    DOOM_DISPLAY_WIDTH=${DOOM_DISPLAY_WIDTH}
    DOOM_DISPLAY_HEIGHT=${DOOM_DISPLAY_HEIGHT}
    DOOM_PANEL_CHUNK_ROWS=${DOOM_PANEL_CHUNK_ROWS_RESOLVED}
    DOOM_ASYNC_AMOLED=$<BOOL:${DOOM_ASYNC_AMOLED}>
    DOOM_HYBRID_CONTROLS=$<BOOL:${DOOM_HYBRID_CONTROLS}>
    DOOM_TOUCH_DPAD=$<BOOL:${DOOM_TOUCH_DPAD}>
    DOOM_TOUCH_DPAD_THUMB_ZONES=$<BOOL:${DOOM_TOUCH_DPAD_THUMB_ZONES}>
    DOOM_TOUCH_DPAD_OVERLAY=$<BOOL:${DOOM_TOUCH_DPAD_OVERLAY}>
    DOOM_BOOT_NEXT_WEAPON=$<BOOL:${DOOM_BOOT_NEXT_WEAPON}>
    DOOM_BOOT_HOLD_STRAFE=$<BOOL:${DOOM_BOOT_HOLD_STRAFE}>
    DOOM_ROLL_STRAFE=$<BOOL:${DOOM_ROLL_STRAFE}>
    DOOM_HANDHELD_AUTO_SAVE_NAMES=$<BOOL:${DOOM_HANDHELD_AUTO_SAVE_NAMES}>
    DOOM_FLASH_SAFE_SAVES=$<BOOL:${DOOM_FLASH_SAFE_SAVES}>
    DOOM_MUSIC_PERCUSSION_PERCENT=${DOOM_MUSIC_PERCUSSION_PERCENT}
    DOOM_MUSIC_SMOOTH_SYNTH=$<BOOL:${DOOM_MUSIC_SMOOTH_SYNTH}>
    DOOM_MUSIC_SPEAKER_MASTERING=$<BOOL:${DOOM_MUSIC_SPEAKER_MASTERING}>
    DOOM_MUSIC_LAB=$<BOOL:${DOOM_MUSIC_LAB}>
    DOOM_MUSIC_LAB_VARIANT=${DOOM_MUSIC_LAB_VARIANT}
    DOOM_AUDIO_SAMPLE_RATE=${DOOM_AUDIO_SAMPLE_RATE}
    DOOM_AUDIO_BUFFER_SAMPLES=${DOOM_AUDIO_BUFFER_SAMPLES}
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

    # Sound effects use the non-blocking DMA/IRQ audio_pio queue. Optional
    # music, when enabled below, mixes through that same bounded queue.
)

if(DOOM_BOOT_NEXT_WEAPON OR DOOM_FLASH_SAFE_SAVES)
    target_compile_definitions(doom PRIVATE
        # A failed core-safety handshake must return an error, not assert. BOOT
        # skips its sample; save leaves the old slot unconfirmed and reports
        # failure rather than entering an unsafe flash write.
        PICO_FLASH_ASSERT_ON_UNSAFE=0
    )
endif()

if(DOOM_BOOT_NEXT_WEAPON)
    if(NOT DOOM_BOOT_WITH_SOUND_EFFECTS)
        target_compile_definitions(doom PRIVATE
            DOOM_BOOT_GATE_AUDIO_DISABLED=1
            DEBUG_NO_SOUND=1
        )
    endif()
endif()

# Only the video flight recorder consumes this value. Keep it source-local so
# changing an iterative capture from 60 to 20 seconds does not rebuild the
# entire embedded Doom engine.
set_property(SOURCE engine/pico/i_video.c APPEND PROPERTY COMPILE_DEFINITIONS
    DOOM_PROFILE_CAPTURE_SECONDS=${DOOM_PROFILE_CAPTURE_SECONDS})

if(DOOM_ENABLE_MUSIC)
    target_compile_definitions(doom PRIVATE DOOM_ENABLE_MUSIC=1)
else()
    target_compile_definitions(doom PRIVATE DEBUG_NO_MUSIC=1)
endif()
if(DOOM_ENABLE_PROFILING)
    target_compile_definitions(doom PRIVATE
        DOOM_ENABLE_PROFILING=1
        # A connected but stalled terminal must not pause gameplay for the SDK's
        # default 500ms USB stdout timeout. Profiling summaries are disposable.
        PICO_STDIO_USB_STDOUT_TIMEOUT_US=2000
        # macOS may enumerate and read CDC without asserting DTR. Treat a ready
        # USB device as connected so the profiling stream is not silently lost.
        PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1
    )
endif()
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
