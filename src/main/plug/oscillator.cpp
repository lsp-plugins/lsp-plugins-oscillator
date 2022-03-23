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

#include <private/plugins/oscillator.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/shared/id_colors.h>

// Maximum size of temporary buffer in samples
#define TMP_BUF_SIZE            1024
#define TRACE_PORT(p)           lsp_trace("  port id=%s", (p)->metadata()->id);

namespace lsp
{
    namespace plugins
    {
        //---------------------------------------------------------------------
        // Plugin factory
        static const meta::plugin_t *plugins[] =
        {
            &meta::oscillator_mono
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            return new oscillator(meta);
        }

        static plug::Factory factory(plugin_factory, plugins, 1);

        //---------------------------------------------------------------------
        oscillator::oscillator(const meta::plugin_t *metadata): plug::Module(metadata)
        {
            nMode                   = 0;
            bMeshSync               = false;
            bBypass                 = false;

            vBuffer                 = NULL;
            vTime                   = NULL;
            vDisplaySamples         = NULL;
            pData                   = NULL;
            pIDisplay               = NULL;

            pIn                     = NULL;
            pOut                    = NULL;
            pBypass                 = NULL;
            pFrequency              = NULL;
            pGain                   = NULL;
            pDCOffset               = NULL;
            pDCRefSc                = NULL;
            pInitPhase              = NULL;
            pModeSc                 = NULL;
            pOversamplerModeSc      = NULL;
            pFuncSc                 = NULL;
            pSquaredSinusoidInv     = NULL;
            pParabolicInv           = NULL;
            pRectangularDutyRatio   = NULL;
            pSawtoothWidth          = NULL;
            pTrapezoidRaiseRatio    = NULL;
            pTrapezoidFallRatio     = NULL;
            pPulsePosWidthRatio     = NULL;
            pPulseNegWidthRatio     = NULL;
            pParabolicWidth         = NULL;
            pOutputMesh             = NULL;
        }

        oscillator::~oscillator()
        {
        }

        void oscillator::destroy()
        {
            if (pData != NULL)
            {
                delete [] pData;
                pData = NULL;
            }
            vBuffer             = NULL;
            vTime               = NULL;
            vDisplaySamples     = NULL;

            if (pIDisplay != NULL)
            {
                pIDisplay->destroy();
                pIDisplay = NULL;
            }
        }

        dspu::fg_function_t oscillator::get_function(size_t function)
        {
            switch (function)
            {
                case meta::oscillator_metadata::SC_FUNC_SINE:
                    return dspu::FG_SINE;
                case meta::oscillator_metadata::SC_FUNC_COSINE:
                    return dspu::FG_COSINE;
                case meta::oscillator_metadata::SC_FUNC_SQUARED_SINE:
                    return dspu::FG_SQUARED_SINE;
                case meta::oscillator_metadata::SC_FUNC_SQUARED_COSINE:
                    return dspu::FG_SQUARED_COSINE;
                case meta::oscillator_metadata::SC_FUNC_RECTANGULAR:
                    return dspu::FG_RECTANGULAR;
                case meta::oscillator_metadata::SC_FUNC_SAWTOOTH:
                    return dspu::FG_SAWTOOTH;
                case meta::oscillator_metadata::SC_FUNC_TRAPEZOID:
                    return dspu::FG_TRAPEZOID;
                case meta::oscillator_metadata::SC_FUNC_PULSETRAIN:
                    return dspu::FG_PULSETRAIN;
                case meta::oscillator_metadata::SC_FUNC_PARABOLIC:
                    return dspu::FG_PARABOLIC;
                case meta::oscillator_metadata::SC_FUNC_BL_RECTANGULAR:
                    return dspu::FG_BL_RECTANGULAR;
                case meta::oscillator_metadata::SC_FUNC_BL_SAWTOOTH:
                    return dspu::FG_BL_SAWTOOTH;
                case meta::oscillator_metadata::SC_FUNC_BL_TRAPEZOID:
                    return dspu::FG_BL_TRAPEZOID;
                case meta::oscillator_metadata::SC_FUNC_BL_PULSETRAIN:
                    return dspu::FG_BL_PULSETRAIN;
                case meta::oscillator_metadata::SC_FUNC_BL_PARABOLIC:
                    return dspu::FG_BL_PARABOLIC;

                default:
                    return dspu::FG_SINE;
            }
            return dspu::FG_SINE;
        }

        dspu::dc_reference_t oscillator::get_dc_reference(size_t reference)
        {
            switch (reference)
            {
                case meta::oscillator_metadata::SC_DC_WAVEDC:
                    return dspu::DC_WAVEDC;
                case meta::oscillator_metadata::SC_DC_ZERO:
                    return dspu::DC_ZERO;
                default:
                    return dspu::DC_WAVEDC;
            }
        }

        dspu::over_mode_t oscillator::get_oversampling_mode(size_t mode)
        {
            switch (mode)
            {
                case meta::oscillator_metadata::SC_OVS_2X:
                    return dspu::OM_LANCZOS_2X2;
                case meta::oscillator_metadata::SC_OVS_3X:
                    return dspu::OM_LANCZOS_3X2;
                case meta::oscillator_metadata::SC_OVS_4X:
                    return dspu::OM_LANCZOS_4X2;
                case meta::oscillator_metadata::SC_OVS_6X:
                    return dspu::OM_LANCZOS_6X2;
                case meta::oscillator_metadata::SC_OVS_8X:
                    return dspu::OM_LANCZOS_8X2;

                case meta::oscillator_metadata::SC_OVS_NONE:
                default:
                    return dspu::OM_NONE;
            }
            return dspu::OM_NONE;
        }

        void oscillator::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            plug::Module::init(wrapper, ports);

            size_t samples  = TMP_BUF_SIZE + meta::oscillator_metadata::HISTORY_MESH_SIZE * 2;
            pData           = new uint8_t[samples * sizeof(float) + DEFAULT_ALIGN];

            uint8_t *ptr    = align_ptr(pData, DEFAULT_ALIGN);
            vBuffer         = reinterpret_cast<float *>(ptr);
            ptr            += TMP_BUF_SIZE * sizeof(float);
            vTime           = reinterpret_cast<float *>(ptr);
            ptr            += meta::oscillator_metadata::HISTORY_MESH_SIZE * sizeof(float);
            vDisplaySamples = reinterpret_cast<float *>(ptr);
            ptr            += meta::oscillator_metadata::HISTORY_MESH_SIZE * sizeof(float);

            lsp_assert(reinterpret_cast<uint8_t *>(ptr) <= &pData[samples * sizeof(float) + DEFAULT_ALIGN]);

            // Make time scale normalized between 0 and 2
            for (size_t n = 0; n < meta::oscillator_metadata::HISTORY_MESH_SIZE; ++n)
                vTime[n] = float(2 * n) / meta::oscillator_metadata::HISTORY_MESH_SIZE;

            size_t port_id          = 0;
            pIn                     = ports[port_id++];
            pOut                    = ports[port_id++];
            pBypass                 = ports[port_id++];
            pFrequency              = ports[port_id++];
            pGain                   = ports[port_id++];
            pDCOffset               = ports[port_id++];
            pDCRefSc                = ports[port_id++];
            pInitPhase              = ports[port_id++];
            pModeSc                 = ports[port_id++];
            pOversamplerModeSc      = ports[port_id++];
            pFuncSc                 = ports[port_id++];
            pSquaredSinusoidInv     = ports[port_id++];
            pParabolicInv           = ports[port_id++];
            pRectangularDutyRatio   = ports[port_id++];
            pSawtoothWidth          = ports[port_id++];
            pTrapezoidRaiseRatio    = ports[port_id++];
            pTrapezoidFallRatio     = ports[port_id++];
            pPulsePosWidthRatio     = ports[port_id++];
            pPulseNegWidthRatio     = ports[port_id++];
            pParabolicWidth         = ports[port_id++];
            pOutputMesh             = ports[port_id++];

            sOsc.init();
        }

        void oscillator::update_sample_rate(long sr)
        {
            sOsc.set_sample_rate(sr);
            sBypass.init(sr);
        }

        void oscillator::ui_activated()
        {
            // Mark mesh needs to be synchronized
            bMeshSync               = true;
        }

        void oscillator::process(size_t samples)
        {
            float *in = pIn->buffer<float>();
            if (in == NULL)
                return;

            float *out = pOut->buffer<float>();
            if (out == NULL)
                return;

            switch (nMode)
            {
                case meta::oscillator_metadata::SC_MODE_ADD:
                {
                    while (samples > 0)
                    {
                        size_t to_do = (samples > TMP_BUF_SIZE) ? TMP_BUF_SIZE : samples;

                        sOsc.process_add(vBuffer, in, to_do);

                        sBypass.process(out, in, vBuffer, to_do);

                        in         += to_do;
                        out        += to_do;
                        samples    -= to_do;
                    }
                }
                break;

                case meta::oscillator_metadata::SC_MODE_MUL:
                {
                    while (samples > 0)
                    {
                        size_t to_do = (samples > TMP_BUF_SIZE) ? TMP_BUF_SIZE : samples;

                        sOsc.process_mul(vBuffer, in, to_do);

                        sBypass.process(out, in, vBuffer, to_do);

                        in         += to_do;
                        out        += to_do;
                        samples    -= to_do;
                    }
                }
                break;

                case meta::oscillator_metadata::SC_MODE_REP:
                {
                    while (samples > 0)
                    {
                        size_t to_do = (samples > TMP_BUF_SIZE) ? TMP_BUF_SIZE : samples;

                        sOsc.process_overwrite(vBuffer, to_do);

                        sBypass.process(out, in, vBuffer, to_do);

                        in         += to_do;
                        out        += to_do;
                        samples    -= to_do;
                    }
                }
                break;

            }

            if (bMeshSync)
            {
                plug::mesh_t *mesh    = pOutputMesh->buffer<plug::mesh_t>();

                if ((mesh != NULL) && (mesh->isEmpty()))
                {
                    dsp::copy(mesh->pvData[0], vTime, meta::oscillator_metadata::HISTORY_MESH_SIZE);
                    dsp::copy(mesh->pvData[1], vDisplaySamples, meta::oscillator_metadata::HISTORY_MESH_SIZE);
                    mesh->data(2, meta::oscillator_metadata::HISTORY_MESH_SIZE);

                    bMeshSync   = false;
                }
            }
        }

        void oscillator::update_settings()
        {
            nMode = pModeSc->value();

            bBypass = pBypass->value() >= 0.5f;
            sBypass.set_bypass(bBypass);

            sOsc.set_parabolic_width(pParabolicWidth->value() / 100);
            sOsc.set_pulsetrain_ratios(pPulsePosWidthRatio->value() / 100, pPulseNegWidthRatio->value() / 100);
            sOsc.set_trapezoid_ratios(pTrapezoidRaiseRatio->value() / 100, pTrapezoidFallRatio->value() / 100);
            sOsc.set_width(pSawtoothWidth->value() / 100);
            sOsc.set_duty_ratio(pRectangularDutyRatio->value() / 100);
            sOsc.set_oversampler_mode(get_oversampling_mode(pOversamplerModeSc->value()));
            sOsc.set_function(get_function(pFuncSc->value()));
            sOsc.set_squared_sinusoid_inversion(pSquaredSinusoidInv->value() >= 0.5f);
            sOsc.set_parabolic_inversion(pParabolicInv->value() >= 0.5f);
            sOsc.set_phase(pInitPhase->value() * M_PI / 180.0f);
            sOsc.set_dc_reference(get_dc_reference(pDCRefSc->value()));
            sOsc.set_dc_offset(pDCOffset->value());
            sOsc.set_frequency(pFrequency->value());
            sOsc.set_amplitude(pGain->value());

            if (sOsc.needs_update())
            {
                sOsc.update_settings();
                bMeshSync   = true;
            }

            sOsc.get_periods(vDisplaySamples, 2, 10, meta::oscillator_metadata::HISTORY_MESH_SIZE);

            // Always query drawing on settings update
            if (pWrapper != NULL)
                pWrapper->query_display_draw();
        }

        bool oscillator::inline_display(plug::ICanvas *cv, size_t width, size_t height)
        {
            // Check proportions
            if (height > (M_RGOLD_RATIO * width))
                height  = M_RGOLD_RATIO * width;

            // Init canvas
            if (!cv->init(width, height))
                return false;
            width       = cv->width();
            height      = cv->height();
            ssize_t cx  = width >> 1;
            ssize_t dx  = width >> 2;
            ssize_t cy  = height >> 1;
            ssize_t dy  = height >> 2;

            // Clear background
            cv->set_color_rgb((bBypass) ? CV_DISABLED : CV_BACKGROUND);
            cv->paint();

            // Draw axis
            cv->set_line_width(1.0);
            cv->set_color_rgb((bBypass) ? CV_SILVER : CV_YELLOW, 0.5f);
            for (size_t i=1; i<4; i += 2)
            {
                cv->line(i*dx, 0, i*dx, height);
                cv->line(0, i*dy, width, i*dy);
            }

            cv->set_color_rgb(CV_WHITE, 0.5f);
            cv->line(cx, 0, cx, height);
            cv->line(0, cy, width, cy);

            // Allocate buffer: t, f(t)
            pIDisplay           = core::IDBuffer::reuse(pIDisplay, 2, width);
            core::IDBuffer *b   = pIDisplay;
            if (b == NULL)
                return false;

            float ni = float(meta::oscillator_metadata::HISTORY_MESH_SIZE) / width; // Normalizing index

            for (size_t j = 0; j < width; ++j)
            {
                // Top left corner has coordinates (0; 0)
                // Bottom right corner has coordinates (width-1; height-1)
                size_t k        = j*ni;
                b->v[0][j]      = j;
                b->v[1][j]      = cy - dy * vDisplaySamples[k];
            }

            // Set colour and draw
            cv->set_color_rgb((bBypass) ? CV_SILVER : CV_MESH);
            cv->set_line_width(2);
            cv->draw_lines(b->v[0], b->v[1], width);

            return true;

        }

        void oscillator::dump(dspu::IStateDumper *v) const
        {
            plug::Module::dump(v);

            v->write_object("sOsc", &sOsc);
            v->write_object("sBypass", &sBypass);

            v->write("nMode", nMode);
            v->write("bMeshSync", bMeshSync);
            v->write("bBypass", bBypass);
            v->write("vBuffer", vBuffer);
            v->write("vTime", vTime);
            v->write("vDisplaySamples", vDisplaySamples);
            v->write("pData", pData);
            v->write("pIDisplay", pIDisplay);

            v->write("pIn", pIn);
            v->write("pOut", pOut);
            v->write("pBypass", pBypass);
            v->write("pFrequency", pFrequency);
            v->write("pGain", pGain);
            v->write("pDCOffset", pDCOffset);
            v->write("pDCRefSc", pDCRefSc);
            v->write("pInitPhase", pInitPhase);
            v->write("pModeSc", pModeSc);
            v->write("pOversamplerModeSc", pOversamplerModeSc);
            v->write("pFuncSc", pFuncSc);
            v->write("pSquaredSinusoidInv", pSquaredSinusoidInv);
            v->write("pParabolicInv", pParabolicInv);
            v->write("pRectangularDutyRatio", pRectangularDutyRatio);
            v->write("pSawtoothWidth", pSawtoothWidth);
            v->write("pTrapezoidRaiseRatio", pTrapezoidRaiseRatio);
            v->write("pTrapezoidFallRatio", pTrapezoidFallRatio);
            v->write("pPulsePosWidthRatio", pPulsePosWidthRatio);
            v->write("pPulseNegWidthRatio", pPulseNegWidthRatio);
            v->write("pParabolicWidth", pParabolicWidth);
            v->write("pOutputMesh", pOutputMesh);
        }
    } // namespace plugins
} // namespace lsp


