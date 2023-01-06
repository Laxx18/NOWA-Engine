
#include "defines.h"
#include "OgreMesh.h"


class DestructibleMeshSplitter
{
public:
	enum EBoolean_Op
	{
		INSIDE,
		OUTSIDE
	};

	/*
	 * splits mesh into fragments
	 * @param inMesh mesh to split
	 * @param fMaxSize maximum allowable size of fragments
	 * @param fRoughness roughness parameter for split plane
	 * @param fResolution size of one texture unit compared to mesh size
	 * @param strCutMaterial material to assign to cutting plane
	 * @return vector of mesh fragments
	 */
	static EXPORTS std::vector<Ogre::v1::MeshPtr> SplitMesh(Ogre::v1::MeshPtr inMesh,
			float fMaxSize, float fRoughness, float fResolution, bool bSmooth,
			unsigned int nRecoveryAttempts,
			bool bCutSurface, const Ogre::String& strCutMaterial);

	/*
	 * splits a mesh into two parts (which can contain several distinct parts though)
	 * @param inMesh mesh to split
	 * @param outMesh1 split part 1
	 * @param outMesh2 split part 2
	 * @param bError flag indicating an error of some kind, initialize this to false
	 * all other params @see SplitMesh
	 *
	 */
	static EXPORTS void Split(Ogre::v1::MeshPtr inMesh, Ogre::v1::MeshPtr& outMesh1, Ogre::v1::MeshPtr& outMesh2,
			float fRoughness, float fResolution,
			bool bCutSurface, Ogre::String strCutMaterial, bool bSmooth, bool& bError,
			Ogre::v1::MeshPtr debugCutPlane=Ogre::v1::MeshPtr());

	/*
	 * performs boolean operation on ogre meshes
	 * @param inMesh1 operand 1
	 * @param inMesh1 operand 2
	 * @param op1 operation on first operand
	 * @param op2 operation on second operand
	 * @param bError flag indicating an error of some kind, initialize this to false
	 * @return result of boolean operation
	 */
	static EXPORTS Ogre::v1::MeshPtr BooleanOp(Ogre::v1::MeshPtr inMesh1, Ogre::v1::MeshPtr inMesh2,
			EBoolean_Op op1, EBoolean_Op op2, bool& bError, bool bIgnoreSecond=false,
			Ogre::v1::MeshPtr debugReTri=Ogre::v1::MeshPtr(), Ogre::v1::MeshPtr debugCutLine=Ogre::v1::MeshPtr());


	static EXPORTS Ogre::v1::MeshPtr loadMesh(Ogre::String strFile);
	static EXPORTS void unloadMesh(Ogre::v1::MeshPtr mesh);
};
