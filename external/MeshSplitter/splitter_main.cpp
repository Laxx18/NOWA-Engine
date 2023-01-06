

/*
 * splitter_main.cpp
 *
 *  Created on: Dec 27, 2011
 *      Author: _1nsane
 */

#include "DestructibleMeshSplitter.h"
#include "OgreMeshExtractor.h"
#include "VertexMerger.h"

 //TESTING
#include "CutMeshGenerator.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "OgreOverlayElement.h"
#include "OgreLodStrategyManager.h"
// #include <ExampleApplication.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreDefaultHardwareBufferManager.h>

#include <ctime>
#include <fstream>
#include <sstream>

void
setMat(Ogre::v1::Entity *pEnt, Ogre::String strMat, Ogre::String resourceGroup)
{
	for (unsigned int iSubMesh = 0; iSubMesh < pEnt->getNumSubEntities(); iSubMesh++)
		pEnt->getSubEntity(iSubMesh)->setMaterialName(strMat, resourceGroup);
}

//class SampleApp : public ExampleApplication
//{
//public:
//	// Basic constructor
//	SampleApp()
//	{
//	}
//	virtual ~SampleApp()
//	{
//	}
//	void setMeshes(std::vector<Ogre::String> vSplinters, Ogre::String strPath)
//	{
//		m_vSplinters = vSplinters; m_strPath = strPath;
//	}
//protected:
//	void chooseSceneManager()
//	{
//		// Create the SceneManager, in this case a generic one
//		mSceneMgr = mRoot->createSceneManager("OctreeSceneManager");
//	}
//
//	// Just override the mandatory create scene method
//	void createScene(void)
//	{
//		// Set default ambient light
//		mSceneMgr->setAmbientLight(ColourValue(0.1f, 0.1f, 0.1f));
//
//		// Set some camera params
//		mCamera->setNearClipDistance(0.1f);
//		mCamera->setFarClipDistance(1000);
//		mCamera->setPosition(0, 0, -10);
//		mCamera->setOrientation(Ogre::Quaternion(1, 0, 1, 0));
//
//		// Light
//		Ogre::Light *mLight = mSceneMgr->createLight("Light0");
//		mLight->setPosition(0, 0, 0);
//		mLight->setDiffuseColour(0.7f, 0.5f, 0.2f);
//		mLight->setSpecularColour(1, 1, 1);
//
//
//		ResourceGroupManager::getSingletonPtr()->addResourceLocation(m_strPath, Ogre::String("FileSystem"));
//		ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
//		//ResourceGroupManager::getSingletonPtr()->initialiseResourceGroup(m_strPath);
//
//		/*Ogre::Entity *ent = mSceneMgr->createEntity("myEnt", meshSplitter.getMesh()->getName());
//		//ent->getSubEntity(0)->setMaterialName(Ogre::String("grey"));
//		ent->setCastShadows(false);*/
//		Ogre::SceneNode* pNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("myNode");
//		pNode->setPosition(0, 0, 0);
//		//pNode->attachObject(ent);
//		pNode->setScale(1, 1, 1);
//
//		Ogre::String strMeshName = OgreMeshExtractor::getRandomMeshName();
//
//		/*		Ogre::MeshPtr cutPlane=Ogre::MeshManager::getSingleton().createManual(strMeshName, "General");
//
//				Ogre::MeshPtr cutlines=Ogre::MeshManager::getSingleton().createManual(Ogre::String("cutlines"), "General");
//
//				Ogre::MeshPtr retriangulated=Ogre::MeshManager::getSingleton().createManual(Ogre::String("retriangulated"), "General");
//		*/
//
//		//meshSplitter.Split(meshSplitter.getMesh(), cutPlane, retriangulated, cutlines, 0.6 );
//
///*
//		Ogre::Entity *cutplaneent = mSceneMgr->createEntity("cutplane", cutPlane.get()->getName());
//		setMat(cutplaneent, Ogre::String("greenish"));
//		cutplaneent->setCastShadows(false);
//		pNode->attachObject(cutplaneent);
//
//		Ogre::Entity *cutlineent = mSceneMgr->createEntity("cutlines", cutlines.get()->getName());
//		setMat(cutlineent, Ogre::String("blueish"));
//		cutlineent->setCastShadows(false);
//		pNode->attachObject(cutlineent);
//
//		Ogre::Entity *retriangulatedent = mSceneMgr->createEntity("retriangulated", retriangulated.get()->getName());
//		setMat(retriangulatedent, Ogre::String("reddish"));
//		retriangulatedent->setCastShadows(false);
//		pNode->attachObject(retriangulatedent);
//*/
//		for (unsigned int iSplinter = 0; iSplinter < m_vSplinters.size(); iSplinter++)
//		{
//			Ogre::MeshPtr pMesh = DestructibleMeshSplitter::loadMesh(m_vSplinters[iSplinter]);
//			pMesh->load();
//			Ogre::String strName = pMesh->getName();
//			Ogre::Entity *ent = mSceneMgr->createEntity(strName, strName);
//			ent->setCastShadows(false);
//			Ogre::SceneNode* pChildNode = pNode->createChildSceneNode(Ogre::String("sub-node") + Ogre::StringConverter::toString(iSplinter));
//			Ogre::Vector3 vCenter = ent->getBoundingBox().getCenter();
//			pChildNode->setPosition(vCenter*1.0f);
//			pChildNode->attachObject(ent);
//		}
//
//		// Create a panel
//		Ogre::OverlayContainer* panel = static_cast<Ogre::OverlayContainer*>(
//			OverlayManager::getSingleton().createOverlayElement("Panel", "HydraxDemoInformationPanel"));
//		panel->setMetricsMode(Ogre::GMM_PIXELS);
//		panel->setPosition(10, 10);
//		panel->setDimensions(400, 400);
//
//		// Create a text area
//		mTextArea = static_cast<Ogre::TextAreaOverlayElement*>(
//			Ogre::OverlayManager::getSingleton().createOverlayElement("TextArea", "HydraxDemoInformationTextArea"));
//		mTextArea->setMetricsMode(Ogre::GMM_PIXELS);
//		mTextArea->setPosition(0, 0);
//		mTextArea->setDimensions(100, 100);
//		mTextArea->setCharHeight(16);
//		mTextArea->setCaption("ICE Mesh Splitter Tester 0.0a");
//		mTextArea->setFontName("BlueHighway");
//		mTextArea->setColourBottom(ColourValue(0.3f, 0.5f, 0.3f));
//		mTextArea->setColourTop(ColourValue(0.5f, 0.7f, 0.5f));
//
//		// Create an overlay, and add the panel
//		Ogre::Overlay* overlay = Ogre::OverlayManager::getSingleton().create("OverlayName");
//		overlay->add2D(panel);
//
//		// Add the text area to the panel
//		panel->addChild(mTextArea);
//
//		// Show the overlay
//		overlay->show();
//	}
//private:
//	std::vector<Ogre::String> m_vSplinters;
//	Ogre::String m_strPath;
//	Ogre::TextAreaOverlayElement* mTextArea;
//};

unsigned int
getMeshByIndex(const std::vector<unsigned int>& viIndexOffsets, unsigned int iIndexIndex)
{
	//subset search
	int iRangeMin = 0;
	int iRangeMax = viIndexOffsets.size() - 2;
	while (iRangeMin != iRangeMax)
	{
		int iSamplePoint = (iRangeMax + iRangeMin) >> 1;
		if (iIndexIndex < viIndexOffsets[iSamplePoint + 1])
			iRangeMax = iSamplePoint;
		else//>=
			iRangeMin = iSamplePoint + 1;
	}
	return iRangeMax;
}

int main(int argc, char **argv)
{
	bool bDisplayVisual = false,
		bDisplayHelp = false,
		bUseRelSize = true,
		bSmooth = false;

	float fSize = 0.2f,
		fRoughness = 1.0f,
		fRelUV = 1.0f;
	int iRandomSeed = 0,
		nRecoveryAttempts = 3;

	Ogre::String strCutMaterial;

	int iCurrArg = 1;
	for (; iCurrArg < argc - 2; iCurrArg++)
	{
		if (!strcmp(argv[iCurrArg], "--help"))
			bDisplayHelp = true;
		else if (!strcmp(argv[iCurrArg], "-visual"))
			bDisplayVisual = true;
		else if (!strcmp(argv[iCurrArg], "-random"))
			iRandomSeed = (int)std::time(NULL);
		else if (!strcmp(argv[iCurrArg], "-smooth"))
			bSmooth = true;
		else if (!strcmp(argv[iCurrArg], "-relsize"))
		{
			iCurrArg++;
			char* tail;
			fSize = (float)strtod(argv[iCurrArg], &tail);
			if (tail == argv[iCurrArg])
			{//error
				bDisplayHelp = true;
				break;
			}
			if (fSize <= 0 || fSize >= 1)
				printf("relsize has to be in interval ]0:1[!\n");
		}
		else if (!strcmp(argv[iCurrArg], "-abssize"))
		{
			bUseRelSize = false;
			iCurrArg++;
			char* tail;
			fSize = (float)strtod(argv[iCurrArg], &tail);
			if (tail == argv[iCurrArg])
			{//error
				bDisplayHelp = true;
				break;
			}
			if (fSize < 0)
				printf("abssize has to be >0!\n");
		}
		else if (!strcmp(argv[iCurrArg], "-recover"))
		{
			iCurrArg++;
			char* tail;
			nRecoveryAttempts = (int)strtod(argv[iCurrArg], &tail);
			if (tail == argv[iCurrArg])
			{//error
				bDisplayHelp = true;
				break;
			}
			if (nRecoveryAttempts < 0)
				printf("recover has to be >0!\n");
		}
		else if (!strcmp(argv[iCurrArg], "-roughness"))
		{
			iCurrArg++;
			char* tail;
			fRoughness = (float)strtod(argv[iCurrArg], &tail);
			if (tail == argv[iCurrArg])
			{//error
				bDisplayHelp = true;
				break;
			}
			if (fRoughness < 0)
				printf("roughness has to be >0!\n");
		}
		else if (!strcmp(argv[iCurrArg], "-rel_uv"))
		{
			iCurrArg++;
			char* tail;
			fRelUV = (float)strtod(argv[iCurrArg], &tail);
			if (tail == argv[iCurrArg])
			{//error
				bDisplayHelp = true;
				break;
			}
			if (fRelUV < 0)
				printf("roughness has to be >0!\n");
		}
		else if (!strcmp(argv[iCurrArg], "-cut_surface"))
		{
			iCurrArg++;
			strCutMaterial = Ogre::String(argv[iCurrArg]);
		}
		else
		{//this should not happen
			bDisplayHelp = true;
			break;
		}
	}
	if (iCurrArg != argc - 2)
		bDisplayHelp = true;

	if (bDisplayHelp)
	{
		printf("Usage:\n\
MeshSplitter [--help] [-visual] [-cut_surface CUT_MATERIAL] [-relsize NUMBER] [-abssize NUMBER] [-roughness NUMBER] [-rel_uv NUMBER] [-random] [-recover NUMBER]  MESH_FILE OUTPUT_DIR\n\
generates fragments of the given OGRE .mesh file, outputs fragment meshes and .cfg file to OUTPUT_DIR\n\
options:\n\
--help        displays this (duh.)\n\
-visual       displays OGRE window in the end, in order to preview the result (.cfg has to be present)\n\
-cut_surface  generate cut surfaces with given material (none will be generated without this parameter)\n\
              DEFAULT: off\n\
-relsize      size of fragments relative to original mesh\n\
              DEFAULT: 0.2\n\
-abssize      absolute maximum size of fragments, overrides -relsize\n\
              DEFAULT: off\n\
-roughness    roughness parameter for cut mesh generation\n\
              DEFAULT: 1.0\n\
-smooth       create smooth cut surface (merge vertex normals)\n\
              DEFAULT: off\n\
-rel_uv       uv scale relative to mesh size\n\
              DEFAULT: 1.0\n\
-random       initialize ramdom number generator to current time\n\
-recover      determines how often the algorithm should try to recover itself from errors before continuing with errors.\n\
              errors are typically generated by numeric instabillities, so the input mesh is simply wiggled around a bit.\n\
              DEFAULT:3");
		return 0;
	}
	//initialize random number generator
	srand(iRandomSeed);

	//see OgreXMLConverter, OGRE's a bit quirky here
	Ogre::LogManager* pLogMgr = new Ogre::LogManager();
	Ogre::ResourceGroupManager* pResMgr = new Ogre::ResourceGroupManager();
	Ogre::Math* pMath = new Ogre::Math();
	Ogre::LodStrategyManager* pLODMgr = new Ogre::LodStrategyManager();
	Ogre::v1::MeshManager* pMeshMgr = new Ogre::v1::MeshManager();
	Ogre::v1::DefaultHardwareBufferManager* pHBMgr = new Ogre::v1::DefaultHardwareBufferManager();

	std::string strFile(argv[iCurrArg]);
	std::string strDestFolder(argv[iCurrArg + 1]);

	int iFolderEnd = std::max((int)strFile.find_last_of('/'), (int)strFile.find_last_of('\\'));
	Ogre::String strSourceFolder = strFile.substr(0, iFolderEnd);
	//5 chars for ".mesh"
	assert(strFile.size() - (iFolderEnd + 1 + 5) > 0);
	Ogre::String strSourceName = strFile.substr(iFolderEnd + 1, strFile.size() - (iFolderEnd + 1 + 5));

	Ogre::v1::MeshPtr pMesh = DestructibleMeshSplitter::loadMesh(strFile);
	if (!pMesh.get())
		return -1;
	float fAbsFragmentSize = fSize;
	if (bUseRelSize)
		fAbsFragmentSize *= pMesh->getBounds().getSize().length();

	/*Ogre::MeshPtr pDeb=Ogre::MeshManager::getSingleton().createManual(OgreMeshExtractor::getRandomMeshName(), "General");
	CutMeshGenerator::CreateRandomCutMesh(pMesh, pDeb, fRoughness, fRelUV, strCutMaterial, bSmooth);*/

	std::vector<Ogre::v1::MeshPtr> vSplinters = DestructibleMeshSplitter::SplitMesh(pMesh,
		fAbsFragmentSize, fRoughness, fRelUV, bSmooth, nRecoveryAttempts,
		strCutMaterial.size() != 0, strCutMaterial);
	//std::vector<Ogre::MeshPtr>(1,pDeb);

//find out which fragments are connected
	std::vector<Ogre::Vector3> vOverallVertices;
	std::vector<unsigned int> viOverallVertexOffsets, viOverallMatchingVertices;

	Ogre::v1::MeshSerializer *pSerializer = new Ogre::v1::MeshSerializer();
	std::vector<Ogre::String> vstrFiles;

	for (unsigned int iSplinter = 0; iSplinter < vSplinters.size(); iSplinter++)
	{
		//write fragment to disk
		Ogre::String strFileName = strSourceName + Ogre::String("_") + Ogre::StringConverter::toString(iSplinter) + Ogre::String(".mesh");
		printf("writing mesh fragment %s\n", strFileName.c_str());
		pSerializer->exportMesh(vSplinters[iSplinter].get(), strDestFolder + strFileName);
		vstrFiles.push_back(strFileName);

		std::vector<Ogre::Vector3> vVertices;
		std::vector<unsigned int> viSubmeshVertexOffsets, viIndices, viSubmeshIndexOffsets, viMatchingVertices;

		OgreMeshExtractor::Extract(vSplinters[iSplinter], vVertices, viSubmeshVertexOffsets,
			viIndices, viSubmeshIndexOffsets);

		DestructibleMeshSplitter::unloadMesh(vSplinters[iSplinter]);

		viOverallVertexOffsets.push_back(vOverallVertices.size());
		vOverallVertices.insert(vOverallVertices.end(), vVertices.begin(), vVertices.end());
	}
	delete pSerializer;

	viOverallMatchingVertices = VertexMerger::GetMatchingIndices(vOverallVertices);
	viOverallVertexOffsets.push_back(vOverallVertices.size());

	std::vector<std::map<unsigned int, std::set<int> > > vmConnections(vSplinters.size());
	for (unsigned int iMesh = 0; iMesh < vSplinters.size(); iMesh++)
	{
		for (unsigned int iVert = viOverallVertexOffsets[iMesh]; iVert < viOverallVertexOffsets[iMesh + 1]; iVert++)
		{
			//get sub-mesh
			int iMatchingVert = viOverallMatchingVertices[iVert];
			unsigned int iOtherMesh = getMeshByIndex(viOverallVertexOffsets, iMatchingVert);
			if (iOtherMesh != iMesh)
			{
				std::map<unsigned int, std::set<int> >::iterator it;
				if ((it = vmConnections[iMesh].find(iOtherMesh)) != vmConnections[iMesh].end())
					it->second.insert(iMatchingVert);
				else
				{
					std::set<int> mySet;
					mySet.insert(iMatchingVert);
					vmConnections[iMesh][iOtherMesh] = mySet;
				}
			}
		}
	}
	//write XML output
	std::string strXMLFile = strDestFolder + strSourceName + Ogre::String(".xml");
	printf("writing XML file to %s\n", strXMLFile.c_str());
	std::ofstream ofs(strXMLFile.c_str());
	ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n";
	ofs << "<mesh_fragments>\n";
	for (unsigned int iFragment = 0; iFragment < vmConnections.size(); iFragment++)
	{
		ofs << "\t<mesh name=\"" << vstrFiles[iFragment].c_str() << "\">\n";
		//;
		for (std::map<unsigned int, std::set<int> >::iterator it = vmConnections[iFragment].begin();
		it != vmConnections[iFragment].end(); it++)
		{
			ofs << "\t\t<connection target=\"" << vstrFiles[it->first].c_str() << "\">\n";
			const std::set<int>& siVertices = it->second;

			Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);
			for (std::set<int>::const_iterator itVert = siVertices.begin(); itVert != siVertices.end(); itVert++)
			{
				Ogre::Vector3 v = vOverallVertices[*itVert];
				vMin.makeFloor(v);
				vMax.makeCeil(v);
			}
			float fSize = (vMax - vMin).length();
			ofs << "\t\t\t<rel_force value=\"" << fSize*fSize << "\"/>\n";
			ofs << "\t\t\t<rel_torque value=\"" << fSize*fSize*fSize << "\"/>\n";
			Ogre::Vector3 vJointCenter = vMin + (vMax - vMin)*.5f;
			ofs << "\t\t\t<joint_center x=\"" << vJointCenter.x <<
				"\" y=\"" << vJointCenter.y <<
				"\" z=\"" << vJointCenter.z << "\"/>\n";

			ofs << "\t\t</connection>\n";
		}
		ofs << "\t</mesh>\n";
	}
	ofs << "</mesh_fragments>";
	ofs.close();


	delete pHBMgr;
	delete pMeshMgr;
	delete pLODMgr;
	delete pMath;
	delete pResMgr;
	delete pLogMgr;

	//if debug visualization is requested:
	//if (bDisplayVisual)
	//{
	//	// Create application object
	//	SampleApp app;
	//	app.setMeshes(vstrFiles, strSourceFolder);

	//	try
	//	{
	//		app.go();
	//	}
	//	catch (Ogre::Exception& e)
	//		//error handling goes here... whatever ;)
	//	{
	//		return -1;
	//	}
	//}
	return 0;
}