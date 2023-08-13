#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D depthTexture2;

void main()
{
	gl_FragDepth = texelFetch( depthTexture2, ivec2( gl_FragCoord.xy ), 0 ).x;
}
