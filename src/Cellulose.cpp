// ---------------------------------------------------------------------------
// Cellulose.cpp — Plugin entry point.
// Uses the AE Smart Render model (PF_Cmd_SMART_PRE_RENDER / PF_Cmd_SMART_RENDER)
// which handles both CPU and GPU paths correctly.
// ---------------------------------------------------------------------------

#include "Cellulose.h"
#include "CelluloseParams.h"
#include "CelluloseAlgorithm.h"
#include "CelluloseCPU.h"
#include "CelluloseGPU.h"


// ---------------------------------------------------------------------------
// About
// ---------------------------------------------------------------------------
static PF_Err About(PF_InData* in_data, PF_OutData* out_data)
{
    PF_SPRINTF(out_data->return_msg,
        "%s v%d.%d\n%s",
        CELLULOSE_NAME,
        CELLULOSE_MAJOR_VERSION,
        CELLULOSE_MINOR_VERSION,
        CELLULOSE_DESCRIPTION);
    return PF_Err_NONE;
}

// ---------------------------------------------------------------------------
// GlobalSetup
// ---------------------------------------------------------------------------
static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data)
{
    out_data->my_version = CELLULOSE_VERSION;

    out_data->out_flags =
        PF_OutFlag_DEEP_COLOR_AWARE |   // support 16-bit
        PF_OutFlag_USE_OUTPUT_EXTENT;

    out_data->out_flags2 =
        PF_OutFlag2_SUPPORTS_SMART_RENDER |
        PF_OutFlag2_FLOAT_COLOR_AWARE     |
        PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG;

#if defined(CELLULOSE_CUDA_ENABLED) || defined(CELLULOSE_METAL_ENABLED)
    out_data->out_flags2 |= PF_OutFlag2_SUPPORTS_GPU_RENDER_F32;
#endif

    return PF_Err_NONE;
}

// ---------------------------------------------------------------------------
// ParamsSetup — register all parameters with After Effects
// ---------------------------------------------------------------------------
static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data,
                          PF_ParamDef* params[])
{
    PF_Err      err = PF_Err_NONE;
    PF_ParamDef def;

#define ADD_FLOAT_SLIDER(idx, name, mn, mx, def_val)     \
    do {                                                   \
        AEFX_CLR_STRUCT(def);                             \
        PF_ADD_FLOAT_SLIDERX(name, mn, mx, mn, mx,       \
            def_val, PF_Precision_HUNDREDTHS, 0, 0, idx); \
    } while(0)

    ADD_FLOAT_SLIDER(PARAM_AMPLITUDE,
        CELLULOSE_PARAM_AMPLITUDE_NAME,
        CELLULOSE_AMPLITUDE_MIN, CELLULOSE_AMPLITUDE_MAX,
        CELLULOSE_AMPLITUDE_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_FREQUENCY,
        CELLULOSE_PARAM_FREQUENCY_NAME,
        CELLULOSE_FREQUENCY_MIN, CELLULOSE_FREQUENCY_MAX,
        CELLULOSE_FREQUENCY_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_IRREGULARITY,
        CELLULOSE_PARAM_IRREGULARITY_NAME,
        CELLULOSE_IRREGULARITY_MIN, CELLULOSE_IRREGULARITY_MAX,
        CELLULOSE_IRREGULARITY_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_EDGE_SENSITIVITY,
        CELLULOSE_PARAM_EDGE_SENSITIVITY_NAME,
        CELLULOSE_EDGE_SENSITIVITY_MIN, CELLULOSE_EDGE_SENSITIVITY_MAX,
        CELLULOSE_EDGE_SENSITIVITY_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_COLOUR_BLEED,
        CELLULOSE_PARAM_COLOUR_BLEED_NAME,
        CELLULOSE_COLOUR_BLEED_MIN, CELLULOSE_COLOUR_BLEED_MAX,
        CELLULOSE_COLOUR_BLEED_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_CANVAS_JITTER,
        CELLULOSE_PARAM_CANVAS_JITTER_NAME,
        CELLULOSE_CANVAS_JITTER_MIN, CELLULOSE_CANVAS_JITTER_MAX,
        CELLULOSE_CANVAS_JITTER_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_EDGE_BLUR,
        CELLULOSE_PARAM_EDGE_BLUR_NAME,
        CELLULOSE_EDGE_BLUR_MIN, CELLULOSE_EDGE_BLUR_MAX,
        CELLULOSE_EDGE_BLUR_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_CRUSH_BLACKS,
        CELLULOSE_PARAM_CRUSH_BLACKS_NAME,
        CELLULOSE_CRUSH_BLACKS_MIN, CELLULOSE_CRUSH_BLACKS_MAX,
        CELLULOSE_CRUSH_BLACKS_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_VIBRANCE,
        CELLULOSE_PARAM_VIBRANCE_NAME,
        CELLULOSE_VIBRANCE_MIN, CELLULOSE_VIBRANCE_MAX,
        CELLULOSE_VIBRANCE_DEFAULT);

    ADD_FLOAT_SLIDER(PARAM_VIBRANCE_FOCUS,
        CELLULOSE_PARAM_VIBRANCE_FOCUS_NAME,
        CELLULOSE_VIBRANCE_FOCUS_MIN, CELLULOSE_VIBRANCE_FOCUS_MAX,
        CELLULOSE_VIBRANCE_FOCUS_DEFAULT);

#undef ADD_FLOAT_SLIDER

    out_data->num_params = PARAM_COUNT;
    return err;
}

// ---------------------------------------------------------------------------
// Helper: checkout all params into a CelluloseParams struct.
// Caller must PF_CHECKIN_PARAM all defs afterward.
// ---------------------------------------------------------------------------
static PF_Err CheckoutParams(
    PF_InData*      in_data,
    PF_ParamDef     (&defs)[10],
    CelluloseParams& out_cp)
{
    PF_Err err = PF_Err_NONE;

    for (int i = 0; i < 10; ++i) AEFX_CLR_STRUCT(defs[i]);

    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_AMPLITUDE,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[0]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_FREQUENCY,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[1]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_IRREGULARITY,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[2]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_EDGE_SENSITIVITY,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[3]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_COLOUR_BLEED,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[4]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_CANVAS_JITTER,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[5]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_EDGE_BLUR,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[6]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_CRUSH_BLACKS,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[7]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_VIBRANCE,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[8]));
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_VIBRANCE_FOCUS,
        in_data->current_time, in_data->time_step, in_data->time_scale, &defs[9]));

    out_cp.amplitude       = static_cast<float>(defs[0].u.fs_d.value);
    out_cp.frequency       = static_cast<float>(defs[1].u.fs_d.value);
    out_cp.irregularity    = static_cast<float>(defs[2].u.fs_d.value);
    out_cp.edgeSensitivity = static_cast<float>(defs[3].u.fs_d.value);
    out_cp.colourBleed     = static_cast<float>(defs[4].u.fs_d.value);
    out_cp.canvasJitter    = static_cast<float>(defs[5].u.fs_d.value);
    out_cp.edgeBlur        = static_cast<float>(defs[6].u.fs_d.value);
    out_cp.crushBlacks     = static_cast<float>(defs[7].u.fs_d.value);
    out_cp.vibrance        = static_cast<float>(defs[8].u.fs_d.value);
    out_cp.vibranceFocus   = static_cast<float>(defs[9].u.fs_d.value);
    out_cp.currentTime     = AETimeToSeconds(in_data);
    out_cp.currentFrame    = (in_data->time_step > 0)
                             ? static_cast<int>(in_data->current_time / in_data->time_step)
                             : 0;

    return err;
}

// ---------------------------------------------------------------------------
// PreRender
// ---------------------------------------------------------------------------
static PF_Err PreRender(PF_InData* in_data, PF_OutData* out_data,
                        PF_PreRenderExtra* extra)
{
    PF_Err            err = PF_Err_NONE;
    PF_RenderRequest  req = extra->input->output_request;
    PF_CheckoutResult in_result;

    AEFX_CLR_STRUCT(in_result);

    req.preserve_rgb_of_zero_alpha = TRUE;

    // Signal that GPU rendering is possible when a supported GPU is present
#if defined(CELLULOSE_CUDA_ENABLED) || defined(CELLULOSE_METAL_ENABLED)
    if (extra->input->what_gpu != PF_GPU_Framework_NONE)
    {
        extra->output->flags |= PF_RenderOutputFlag_GPU_RENDER_POSSIBLE;
    }
#endif

    ERR(extra->cb->checkout_layer(
        in_data->effect_ref,
        PARAM_INPUT, PARAM_INPUT,
        &req,
        in_data->current_time,
        in_data->time_step,
        in_data->time_scale,
        &in_result));

    if (!err)
    {
        UnionLRect(&in_result.result_rect,     &extra->output->result_rect);
        UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
    }

    return err;
}

// ---------------------------------------------------------------------------
// SmartRender — dispatches to GPU or CPU path
// ---------------------------------------------------------------------------
static PF_Err SmartRender(PF_InData* in_data, PF_OutData* out_data,
                          PF_SmartRenderExtra* extra)
{
    PF_Err err  = PF_Err_NONE;
    PF_Err err2 = PF_Err_NONE;

    PF_EffectWorld* input_world  = nullptr;
    PF_EffectWorld* output_world = nullptr;

    // Checkout layer pixels and output world
    ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, PARAM_INPUT, &input_world));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_world));

    if (!err && input_world && output_world)
    {
        // Checkout parameters
        PF_ParamDef pdefs[10];
        CelluloseParams cp{};
        ERR(CheckoutParams(in_data, pdefs, cp));

        if (!err)
        {
            cp.width  = input_world->width;
            cp.height = input_world->height;

            const PF_GPU_Framework what_gpu = extra->input->what_gpu;
            // -----------------------------------------------------------------
            // Dispatch to GPU or CPU renderer
            // -----------------------------------------------------------------
            if (what_gpu != PF_GPU_Framework_NONE)
            {
                // --- GPU path ---
                cp.bitDepth = 32; // GPU worlds are always float32
                err = CelluloseGPU::Render(
                    in_data, out_data, input_world, output_world, cp,
                    what_gpu, extra->input->gpu_data, extra->input->device_index);
            }

            if (err != PF_Err_NONE || what_gpu == PF_GPU_Framework_NONE)
            {
                // --- CPU fallback ---
                err = PF_Err_NONE;
                cp.bitDepth = PF_WORLD_IS_DEEP(output_world) ? 16 : 8;
                err = CelluloseCPU::Render(in_data, input_world, output_world, cp);
            }
        }

        // Always checkin params
        for (int i = 0; i < 10; ++i) ERR2(PF_CHECKIN_PARAM(in_data, &pdefs[i]));
    }

    return err;
}

// ---------------------------------------------------------------------------
// EffectMain — top-level dispatch
// ---------------------------------------------------------------------------
extern "C" DllExport
PF_Err EffectMain(
    PF_Cmd       cmd,
    PF_InData*   in_data,
    PF_OutData*  out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output,
    void*        extra)
{
    PF_Err err = PF_Err_NONE;

    try
    {
        switch (cmd)
        {
        case PF_Cmd_ABOUT:
            err = About(in_data, out_data);
            break;

        case PF_Cmd_GLOBAL_SETUP:
            err = GlobalSetup(in_data, out_data);
            break;

        case PF_Cmd_PARAMS_SETUP:
            err = ParamsSetup(in_data, out_data, params);
            break;

        case PF_Cmd_SMART_PRE_RENDER:
            err = PreRender(in_data, out_data,
                            reinterpret_cast<PF_PreRenderExtra*>(extra));
            break;

        case PF_Cmd_SMART_RENDER:
            err = SmartRender(in_data, out_data,
                              reinterpret_cast<PF_SmartRenderExtra*>(extra));
            break;

        case PF_Cmd_GPU_DEVICE_SETUP:
        case PF_Cmd_GPU_DEVICE_SETDOWN:
            // No persistent GPU state needed; AE manages device lifetime.
            break;

        case PF_Cmd_SEQUENCE_SETUP:
        case PF_Cmd_SEQUENCE_RESETUP:
        case PF_Cmd_SEQUENCE_FLATTEN:
        case PF_Cmd_SEQUENCE_SETDOWN:
            break;

        default:
            break;
        }
    }
    catch (PF_Err& thrown_err)
    {
        err = thrown_err;
    }
    catch (...)
    {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    return err;
}
