///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	//FLOOR
	bReturn = CreateGLTexture(
		"../../Utilities/textures/pavers.jpg",
		"floor"
	);
	//FLOOR
	bReturn = CreateGLTexture(
		"../../Utilities/textures/dirty.jpg",
		"floor2"
	);
	
	//TableLeg
	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"plank"
	);
	//TableTop
	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"desk"
	);
	//Backwall
	bReturn = CreateGLTexture(
		"../../Utilities/textures/slimBrick.jpg",
		"bDrop"
		
  	);
	//Book5
	bReturn = CreateGLTexture(
		"../../Utilities/textures/book011.jpg",
		"Book5"
		
	);
	//Books
	bReturn = CreateGLTexture(
		"../../Utilities/textures/book022.jpg",
		"Books"

	);
	//Monitor Plastic
	bReturn = CreateGLTexture(
		"../../Utilities/textures/plastic.jpg",
		"plastic"

	);
	//Monitor screen
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Mons2.jpg",
		"screen"

	);
	//keyboard Drawer
	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"wood"

	);
	//keyboard texture
	bReturn = CreateGLTexture(
		"../../Utilities/textures/kb1.jpg",
		"KB1"

	);
	//poster texture
	bReturn = CreateGLTexture(
		"../../Utilities/textures/tuckersoft.jpg",
		"poster"

	);


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
/***********************************************************
*DefineObjectMaterials()
*
* This method is used for configuring the various material
* settings for all of the objects within the 3D scene.
* **********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/


	///////////////// GOLD   ////////////////////
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	/////////////////  CEMENT /////////////////
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	///////////////// WOOD ////////////////////
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.5;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);
	///////////////// TILE ////////////////////
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	///////////////// glass ////////////////////
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	///////////////// CLAY ////////////////////
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	clayMaterial.ambientStrength = 0.2f;
	clayMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	clayMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light 0: Upper right light
	m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 10.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.10f, 0.09f, 0.08f);  //  warm tone
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.3f, 0.3f, 0.3f);  // Lower  intensity
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.002f);

	// Light 1: Upper left light
	m_pShaderManager->setVec3Value("lightSources[1].position", -10.0f, 10.4f, -9.5f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.12f, 0.09f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.35f, 0.33f, 0.30f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.3f, 0.3f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.02f);

	// Light 2: Center overhead (above the glass table)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 10.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.10f, 0.09f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.1f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 54.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.01f); 

	// Light 3: Fill light
	m_pShaderManager->setVec3Value("lightSources[3].position", 10.0f, 0.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.10f, 0.09f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.35f, 0.33f, 0.30f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.015f);


}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// load the textures for the 3D scene
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	LoadSceneTextures();
	// LoadShapes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	//FLOOR
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes. Floor					***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(18.0f, 1.0f, 7.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.2f, -2.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.5, 1, 1);
	SetShaderTexture("wood");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	//FLOOR2
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes. Floor					***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.5f, 1.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 0.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.5, 1, 1);
	SetShaderTexture("floor");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/*******************************************************/
	/***                  backDrop                  	***/
	/*****************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 10.2f, 8.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 1, 1, 1);
	SetShaderTexture("bDrop");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/*******************************************************/
	/***                  poster                  	***/
	/*****************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 6.2f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 6.0f, -10.00f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 1, 1, 1);
	SetShaderTexture("poster");
	//SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	
	/*******************************************************************/
	/***                  TableTop             	                	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.4f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 2.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("wood");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*******************************************************/
	/***                  LEG A                     	***/
	/*****************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 1.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 1, 1, 1);
	SetShaderTexture("plank");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*******************************************************/
	/***                  LEG  Detail              	***/
	/*****************************************************/
	// set the XYZ scale for the mesh
	
	
		
	for (int i = 0; i <= 1; i++) {
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;
		if (i == 0) {
			// set the XYZ position for the mesh
			positionXYZ = glm::vec3(-3.18f, 1.3f, -2.0f);
		}
		else {
			positionXYZ = glm::vec3(4.18f, 1.3f, -2.0f);
		}
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.2, 1, 1, 1);
		SetShaderTexture("screen");
		SetShaderMaterial("glass");
		// draw the mesh with transformation values
		m_basicMeshes->DrawTorusMesh();
	}
	/*******************************************************/
	/***                  LEG B                     	***/
	/*****************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.9f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 1.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 1, 1, 1);
	SetShaderTexture("plank");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***************************BOOK Generator***************************/
	/***************************   Eric L Foster ************************/
	/*                        									       */
	/*Creating one section of code at a time for each of the books 
	 would result in over 150 lines of code; using a combination of a 
	 for loop and ternary operators has reduced the lines of 
	 code to under 30 lines */
	/******************************************************************/
	float yRot = 10.0f, 
		  yPos = 2.8f,
	      xPos = -2.5f,
	      xScale = 0.7f,
		  zScale = 0.9;
	for (int i = 0; i <= 4; i++) {
		
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(xScale, 0.1f, zScale);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = yRot;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(xPos, yPos, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Ternary Operators to set the appropriate Tex and Mat
		(i  == 4) ? SetShaderTexture("Book5") : SetShaderTexture("Books");
		(i % 2 == 0) ? SetShaderMaterial("wood"): SetShaderMaterial("clay");
		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		//Stack the books
		yPos += 0.10;
		//ternary operators to slightly offset and resize all books.
		//            Even           Odd
		(i % 2 == 0)? yRot   += 4.23 : yRot -= 1.03;
		(i % 2 == 0)? xScale += 0.13 : xScale -= .15;
		(i % 2 == 0)? zScale += 0.12 : zScale -= .10;
			

	}
	
	/*                    MONITOR COMPONETS                            */


	/*******************************************************************/
	/***                  Monitor Base                             	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.1f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 2.7f, -2.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/*******************************************************************/
	/***                  Monitor ARM                             	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 2.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 3.9f, -2.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***                  Monitor BRACKET                          	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.09f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 5.0f, -2.09f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***                  Monitor                                	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 2.5f, 0.20f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 5.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("cement");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***                  Monitor  Screen                          	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.8f, 2.3f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 5.0f, -1.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("screen");
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	
	/********************************************End of Monitor Componets **************************/
	/*******************************************************************/
	/***                  KeyBoard	Drawer                         	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.3f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 22.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 2.2f, -0.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***                  KeyBoard	                            	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 0.1f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 22.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 2.4f, -0.18f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("KB1");
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*******************************************************************/
	/***                  LightFixture                            	***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 10.5f, -9.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

