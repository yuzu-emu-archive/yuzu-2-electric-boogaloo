// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <type_traits>
#include <glad/glad.h>
#include "video_core/engines/maxwell_3d.h"

namespace OpenGL {

class OpenGLState {
public:
    struct {
        bool test_enabled = false; // GL_STENCIL_TEST
        struct {
            GLenum test_func = GL_ALWAYS;         // GL_STENCIL_FUNC
            GLint test_ref = 0;                   // GL_STENCIL_REF
            GLuint test_mask = 0xFFFFFFFF;        // GL_STENCIL_VALUE_MASK
            GLuint write_mask = 0xFFFFFFFF;       // GL_STENCIL_WRITEMASK
            GLenum action_stencil_fail = GL_KEEP; // GL_STENCIL_FAIL
            GLenum action_depth_fail = GL_KEEP;   // GL_STENCIL_PASS_DEPTH_FAIL
            GLenum action_depth_pass = GL_KEEP;   // GL_STENCIL_PASS_DEPTH_PASS
        } front, back;
    } stencil;

    struct Blend {
        bool enabled = false;              // GL_BLEND
        GLenum rgb_equation = GL_FUNC_ADD; // GL_BLEND_EQUATION_RGB
        GLenum a_equation = GL_FUNC_ADD;   // GL_BLEND_EQUATION_ALPHA
        GLenum src_rgb_func = GL_ONE;      // GL_BLEND_SRC_RGB
        GLenum dst_rgb_func = GL_ZERO;     // GL_BLEND_DST_RGB
        GLenum src_a_func = GL_ONE;        // GL_BLEND_SRC_ALPHA
        GLenum dst_a_func = GL_ZERO;       // GL_BLEND_DST_ALPHA
    };
    std::array<Blend, Tegra::Engines::Maxwell3D::Regs::NumRenderTargets> blend;

    struct {
        bool enabled = false;
    } independant_blend;

    static constexpr std::size_t NumSamplers = 32 * 5;
    static constexpr std::size_t NumImages = 8 * 5;
    std::array<GLuint, NumSamplers> textures = {};
    std::array<GLuint, NumSamplers> samplers = {};
    std::array<GLuint, NumImages> images = {};

    struct {
        GLuint read_framebuffer = 0; // GL_READ_FRAMEBUFFER_BINDING
        GLuint draw_framebuffer = 0; // GL_DRAW_FRAMEBUFFER_BINDING
        GLuint shader_program = 0;   // GL_CURRENT_PROGRAM
        GLuint program_pipeline = 0; // GL_PROGRAM_PIPELINE_BINDING
    } draw;

    struct {
        GLenum origin = GL_LOWER_LEFT;
        GLenum depth_mode = GL_NEGATIVE_ONE_TO_ONE;
    } clip_control;

    GLuint renderbuffer{}; // GL_RENDERBUFFER_BINDING

    OpenGLState();

    /// Get the currently active OpenGL state
    static OpenGLState GetCurState() {
        return cur_state;
    }

    /// Apply this state as the current OpenGL state
    void Apply();

    void ApplyFramebufferState();
    void ApplyShaderProgram();
    void ApplyProgramPipeline();
    void ApplyStencilTest();
    void ApplyTargetBlending(std::size_t target, bool force);
    void ApplyGlobalBlending();
    void ApplyBlending();
    void ApplyTextures();
    void ApplySamplers();
    void ApplyImages();
    void ApplyClipControl();
    void ApplyRenderBuffer();

    /// Resets any references to the given resource
    OpenGLState& UnbindTexture(GLuint handle);
    OpenGLState& ResetSampler(GLuint handle);
    OpenGLState& ResetProgram(GLuint handle);
    OpenGLState& ResetPipeline(GLuint handle);
    OpenGLState& ResetFramebuffer(GLuint handle);
    OpenGLState& ResetRenderbuffer(GLuint handle);

private:
    static OpenGLState cur_state;
};
static_assert(std::is_trivially_copyable_v<OpenGLState>);

} // namespace OpenGL
