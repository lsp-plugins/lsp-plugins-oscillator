/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-oscillator
 * Created on: 3 авг. 2021 г.
 *
 * lsp-plugins-oscillator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-oscillator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-oscillator. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/oscillator.h>

#define LSP_PLUGINS_OSCILLATOR_VERSION_MAJOR       1
#define LSP_PLUGINS_OSCILLATOR_VERSION_MINOR       0
#define LSP_PLUGINS_OSCILLATOR_VERSION_MICRO       15

#define LSP_PLUGINS_OSCILLATOR_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_OSCILLATOR_VERSION_MAJOR, \
        LSP_PLUGINS_OSCILLATOR_VERSION_MINOR, \
        LSP_PLUGINS_OSCILLATOR_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        static const int plugin_classes[]           = { C_OSCILLATOR, -1 };
        static const int clap_features[]            = { CF_AUDIO_EFFECT, -1 };

        static const port_item_t sc_func[] =
        {
            { "Sine",                       "oscillator.sin" },
            { "Cosine",                     "oscillator.cos" },
            { "Squared Sine",               "oscillator.sqr_sin" },
            { "Squared Cosine",             "oscillator.sqr_cos" },
            { "Rectangular",                "oscillator.rect" },
            { "Sawtooth",                   "oscillator.saw" },
            { "Trapezoid",                  "oscillator.trap" },
            { "Pulsetrain",                 "oscillator.pulse" },
            { "Parabolic",                  "oscillator.para" },
            { "Band Limited Rectangular",   "oscillator.bl_rect" },
            { "Band Limited Sawtooth",      "oscillator.bl_saw" },
            { "Band Limited Trapezoid",     "oscillator.bl_trap" },
            { "Band Limited Pulsetrain",    "oscillator.bl_pulse" },
            { "Band Limited Parabolic",     "oscillator.bl_para" },
            { NULL, NULL }
        };

        static const port_item_t sc_dc_ref[] =
        {
            { "Wave DC",                    "oscillator.wave_dc" },
            { "Zero DC",                    "oscillator.zero_dc" },
            { NULL, NULL }
        };

        static const port_item_t sc_mode[] =
        {
            { "Add",                        "oscillator.add" },
            { "Multiply",                   "oscillator.mul" },
            { "Replace",                    "oscillator.rep" },
            { NULL, NULL }
        };

        static const port_item_t sc_oversampler_mode[] =
        {
            { "None",                       "oversampler.none" },
            { "x2",                         "oversampler.normal.x2" },
            { "x3",                         "oversampler.normal.x3" },
            { "x4",                         "oversampler.normal.x4" },
            { "x6",                         "oversampler.normal.x6" },
            { "x8",                         "oversampler.normal.x8" },
            { NULL, NULL }
        };

        static const port_t oscillator_ports[] =
        {
            PORTS_MONO_PLUGIN,
            BYPASS,
            LOG_CONTROL("freq", "Frequency", U_HZ, oscillator_metadata::FREQUENCY),
            AMP_GAIN10("gain", "Output gain", 1.0f),
            CONTROL("dcoff", "DC Offset", U_NONE, oscillator_metadata::DCOFFSET),
            COMBO("scr", "DC Reference", oscillator_metadata::SC_DC_DFL, sc_dc_ref),
            CYC_CONTROL("iniph", "Initial Phase", U_DEG, oscillator_metadata::INITPHASE),
            COMBO("scm", "Operation Mode", oscillator_metadata::SC_MODE_DFL, sc_mode),
            COMBO("scom", "Oversampler Mode", oscillator_metadata::SC_OVS_DFL, sc_oversampler_mode),
            COMBO("scf", "Function", oscillator_metadata::SC_FUNC_DFL, sc_func),
            SWITCH("invss", "Invert Squared Sinusoids", 0.0f),
            SWITCH("invps", "Invert Parabolic Signal", 0.0f),
            PERCENTS("rdtrt", "Duty Ratio", 50.0f, 0.025f),
            PERCENTS("swdth", "Width", 50.0f, 0.025f),
            PERCENTS("trsrt", "Raise Ratio", 25.0f, 0.025f),
            PERCENTS("tflrt", "Fall Ratio", 25.0f, 0.025f),
            PERCENTS("tpwrt", "Positive Width", 25.0f, 0.025f),
            PERCENTS("tnwrt", "Negative Width", 25.0f, 0.025f),
            PERCENTS("pwdth", "Width", 100.0f, 0.025f),
            MESH("ow", "Output wave", 2, oscillator_metadata::HISTORY_MESH_SIZE),

            PORTS_END
        };

        const meta::bundle_t oscillator_bundle =
        {
            "oscillator",
            "Oscillator",
            B_GENERATORS,
            "D8evQrkY8nQ",
            "This plugin implements a simple yet flexible utility signal generator.\nIt can be used to provide reference waves in replacement, additive and\nmultiplicative (modulation) mode."
        };

        const meta::plugin_t oscillator_mono =
        {
            "Oszillator Mono",
            "Oscillator Mono",
            "O1M", // Oscillator x1 Mono
            &developers::s_tronci,
            "oscillator_mono",
            LSP_LV2_URI("oscillator_mono"),
            LSP_LV2UI_URI("oscillator_mono"),
            "sntq",
            LSP_LADSPA_OSCILLATOR_BASE + 0,
            LSP_LADSPA_URI("oscillator_mono"),
            LSP_CLAP_URI("oscillator_mono"),
            LSP_PLUGINS_OSCILLATOR_VERSION,
            plugin_classes,
            clap_features,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            oscillator_ports,
            "util/oscillator_mono.xml",
            NULL,
            mono_plugin_port_groups,
            &oscillator_bundle
        };
    } /* namespace meta */
} /* namespace lsp */
