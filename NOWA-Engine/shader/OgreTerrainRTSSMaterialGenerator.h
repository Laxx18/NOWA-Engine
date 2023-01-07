#ifndef OgreTerrainRTSSMaterialGenerator_H
#define OgreTerrainRTSSMaterialGenerator_H

#include <OgreTerrainMaterialGeneratorA.h>

namespace Ogre
{
	/**
	* Custom terrain material generator compatible with the RTSS PSSM shadow
	* technique. A TerrainMaterialGenerator which can cope with normal mapped, specular mapped
	*  terrain.
	*  @note Requires the Cg plugin to render correctly
	**/
	class TerrainRTSSMaterialGenerator : public TerrainMaterialGeneratorA
	{
	public:
		TerrainRTSSMaterialGenerator();
		~TerrainRTSSMaterialGenerator();

		/** Shader model 2 profile target.
		*/
		class CustomSM2Profile : public TerrainMaterialGeneratorA::SM2Profile
		{
		public:
			CustomSM2Profile(TerrainMaterialGenerator* parent, const String& name, const String& desc);
			~CustomSM2Profile();

			MaterialPtr generate(const Terrain* terrain);
			MaterialPtr generateForCompositeMap(const Terrain* terrain);

		protected:
			virtual void addTechnique(const MaterialPtr& mat, const Terrain* terrain, TechniqueType tt);

			/// Utility class to help with generating shaders for Cg / HLSL.
			class CustomShaderHelperCg : public TerrainMaterialGeneratorA::SM2Profile::ShaderHelperCg
			{
			public:
				virtual HighLevelGpuProgramPtr generateVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
			protected:
				virtual void generateVpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, std::stringstream& outStream);
				virtual void generateVpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, std::stringstream& outStream);
				virtual void generateVertexProgramSource(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, std::stringstream& outStream);

				virtual void defaultVpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog);
				virtual uint generateVpDynamicShadowsParams(uint texCoordStart, const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, std::stringstream& outStream);
				virtual void generateVpDynamicShadows(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, std::stringstream& outStream);
			};
		};

	};

}

#endif // OgreTerrainRTSSMaterialGenerator_H
