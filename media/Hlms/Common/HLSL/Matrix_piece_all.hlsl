
@property( !hlms_readonly_is_tex )

@piece( Common_Matrix_DeclUnpackMatrix4x4 )
float4x4 UNPACK_MAT4( StructuredBuffer<float4> matrixBuf, uint pixelIdx )
{
	float4 row1 = matrixBuf[int(pixelIdx << 2u)];
	float4 row2 = matrixBuf[int((pixelIdx << 2u) + 1u)];
	float4 row3 = matrixBuf[int((pixelIdx << 2u) + 2u)];
	float4 row4 = matrixBuf[int((pixelIdx << 2u) + 3u)];

	return transpose( float4x4( row1, row2, row3, row4 ) );
}
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
float4x3 UNPACK_MAT4x3( StructuredBuffer<float4> matrixBuf, uint pixelIdx )
{
	float4 row1 = matrixBuf[int(pixelIdx << 2u)];
	float4 row2 = matrixBuf[int((pixelIdx << 2u) + 1u)];
	float4 row3 = matrixBuf[int((pixelIdx << 2u) + 2u)];

	return transpose( float3x4( row1, row2, row3 ) );
}
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
ogre_float4x3 loadOgreFloat4x3( StructuredBuffer<float4> matrixBuf, uint offsetIdx )
{
	float4 row1 = matrixBuf[int(offsetIdx)];
	float4 row2 = matrixBuf[int(offsetIdx) + 1u];
	float4 row3 = matrixBuf[int(offsetIdx) + 2u];

	return transpose( float3x4( row1, row2, row3 ) );
}

#define makeOgreFloat4x3( row0, row1, row2 ) transpose( float3x4( row0, row1, row2 ) )
@end

@else

@piece( Common_Matrix_DeclUnpackMatrix4x4 )
float4x4 UNPACK_MAT4( Buffer<float4> matrixBuf, uint pixelIdx )
{
	float4 row1 = matrixBuf.Load( int((pixelIdx) << 2u) );
	float4 row2 = matrixBuf.Load( int(((pixelIdx) << 2u) + 1u) );
	float4 row3 = matrixBuf.Load( int(((pixelIdx) << 2u) + 2u) );
	float4 row4 = matrixBuf.Load( int(((pixelIdx) << 2u) + 3u) );

	return transpose( float4x4( row1, row2, row3, row4 ) );
}
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
float4x3 UNPACK_MAT4x3( Buffer<float4> matrixBuf, uint pixelIdx )
{
	float4 row1 = matrixBuf.Load( int((pixelIdx) << 2u) );
	float4 row2 = matrixBuf.Load( int(((pixelIdx) << 2u) + 1u) );
	float4 row3 = matrixBuf.Load( int(((pixelIdx) << 2u) + 2u) );

	return transpose( float3x4( row1, row2, row3 ) );
}
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
ogre_float4x3 loadOgreFloat4x3( Buffer<float4> matrixBuf, uint offsetIdx )
{
	float4 row1 = matrixBuf.Load( int(offsetIdx) );
	float4 row2 = matrixBuf.Load( int(offsetIdx) + 1u );
	float4 row3 = matrixBuf.Load( int(offsetIdx) + 2u );

	return transpose( float3x4( row1, row2, row3 ) );
}

#define makeOgreFloat4x3( row0, row1, row2 ) transpose( float3x4( row0, row1, row2 ) )
@end

@end
