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

#ifndef PRIVATE_PLUGINS_OSCILLATOR_H_
#define PRIVATE_PLUGINS_OSCILLATOR_H_

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/IDBuffer.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/dsp-units/util/Oscillator.h>

#include <private/meta/oscillator.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Oscillator Plugin Series
         */
        class oscillator: public plug::Module
        {
            protected:
                dspu::Oscillator     sOsc;
                dspu::Bypass         sBypass;
                size_t               nMode;
                bool                 bMeshSync;
                bool                 bBypass;

                float               *vBuffer;
                float               *vTime;
                float               *vDisplaySamples;
                uint8_t             *pData;
                core::IDBuffer      *pIDisplay;      // Inline display buffer

                plug::IPort         *pIn;
                plug::IPort         *pOut;
                plug::IPort         *pBypass;
                plug::IPort         *pFrequency;
                plug::IPort         *pGain;
                plug::IPort         *pDCOffset;
                plug::IPort         *pDCRefSc;
                plug::IPort         *pInitPhase;
                plug::IPort         *pModeSc;
                plug::IPort         *pOversamplerModeSc;
                plug::IPort         *pFuncSc;
                plug::IPort         *pSquaredSinusoidInv;
                plug::IPort         *pParabolicInv;
                plug::IPort         *pRectangularDutyRatio;
                plug::IPort         *pSawtoothWidth;
                plug::IPort         *pTrapezoidRaiseRatio;
                plug::IPort         *pTrapezoidFallRatio;
                plug::IPort         *pPulsePosWidthRatio;
                plug::IPort         *pPulseNegWidthRatio;
                plug::IPort         *pParabolicWidth;
                plug::IPort         *pOutputMesh;

            protected:
                static dspu::fg_function_t      get_function(size_t function);
                static dspu::dc_reference_t     get_dc_reference(size_t reference);
                static dspu::over_mode_t        get_oversampling_mode(size_t mode);

            public:
                explicit oscillator(const meta::plugin_t *metadata);
                virtual ~oscillator();

                virtual void        init(plug::IWrapper *wrapper);
                virtual void        destroy();

            public:
                virtual void        process(size_t samples);
                virtual void        update_settings();
                virtual void        update_sample_rate(long sr);

                virtual bool        inline_display(plug::ICanvas *cv, size_t width, size_t height);
                virtual void        ui_activated();
                virtual void        dump(dspu::IStateDumper *v) const;
        };
    } // namespace plugins
} // namespace lsp

#endif /* PRIVATE_PLUGINS_OSCILLATOR_H_ */
