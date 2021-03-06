#include "FileSystem.h"
#include "../Common/CommonMacro.inl"
#include "../Component/MeshDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../Core/InnoLogger.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../Core/IOService.h"
#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

using SceneLoadingCallback = std::pair<std::function<void()>*, int32_t>;

namespace InnoFileSystemNS
{
	bool saveScene(const char* fileName);
	bool prepareForLoadingScene(const char* fileName);
	bool loadScene(const char* fileName);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	std::unordered_map<uint32_t, std::string> m_componentIDToNameLUT;
	std::unordered_map<std::string, uint32_t> m_componentNameToIDLUT;

	std::vector<SceneLoadingCallback> m_sceneLoadingStartCallbacks;
	std::vector<SceneLoadingCallback> m_sceneLoadingFinishCallbacks;

	std::atomic<bool> m_isLoadingScene = false;
	std::atomic<bool> m_prepareForLoadingScene = false;

	std::string m_nextLoadingScene;
	std::string m_currentScene;
}

using namespace InnoFileSystemNS;

bool InnoFileSystemNS::saveScene(const char* fileName)
{
	if (!strcmp(fileName, ""))
	{
		return JSONWrapper::saveScene(m_currentScene.c_str());
	}
	else
	{
		return JSONWrapper::saveScene(fileName);
	}
}

bool InnoFileSystemNS::prepareForLoadingScene(const char* fileName)
{
	if (!m_isLoadingScene)
	{
		if (m_currentScene == fileName)
		{
			InnoLogger::Log(LogLevel::Warning, "FileSystem: Scene ", fileName, " has already loaded now.");
			return true;
		}
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool InnoFileSystemNS::loadScene(const char* fileName)
{
	m_currentScene = fileName;

	std::sort(m_sceneLoadingStartCallbacks.begin(), m_sceneLoadingStartCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second > B.second;
		});

	std::sort(m_sceneLoadingFinishCallbacks.begin(), m_sceneLoadingFinishCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second > B.second;
		});

	for (auto& i : m_sceneLoadingStartCallbacks)
	{
		(*i.first)();
	}

	JSONWrapper::loadScene(fileName);

	for (auto& i : m_sceneLoadingFinishCallbacks)
	{
		(*i.first)();
	}

	m_isLoadingScene = false;

	InnoLogger::Log(LogLevel::Success, "FileSystem: Scene ", fileName, " has been loaded.");

	return true;
}

bool InnoFileSystem::setup()
{
	IOService::setupWorkingDirectory();

	json j;

	JSONWrapper::loadJsonDataFromDisk("..//Source//Engine//Common//ComponentType.json", j);

	for (auto i : j)
	{
		m_componentIDToNameLUT.emplace(i["ID"], i["Name"]);
		m_componentNameToIDLUT.emplace(i["Name"], i["ID"]);
	}

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoFileSystem::initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "FileSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: Object is not created!");
		return false;
	}
}

bool InnoFileSystem::update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		if (m_prepareForLoadingScene)
		{
			m_prepareForLoadingScene = false;
			m_isLoadingScene = true;
			g_pModuleManager->getTaskSystem()->waitAllTasksToFinish();
			InnoFileSystemNS::loadScene(m_nextLoadingScene.c_str());
			GetComponentManager(VisibleComponent)->LoadAssetsForComponents();
		}
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	return true;
}

bool InnoFileSystem::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus InnoFileSystem::getStatus()
{
	return m_ObjectStatus;
}

std::string InnoFileSystem::getWorkingDirectory()
{
	return IOService::getWorkingDirectory();
}

std::vector<char> InnoFileSystem::loadFile(const char* filePath, IOMode openMode)
{
	return IOService::loadFile(filePath, openMode);
}

bool InnoFileSystem::saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode)
{
	return IOService::saveFile(filePath, content, saveMode);
}

const char* InnoFileSystem::getComponentTypeName(uint32_t typeID)
{
	auto l_result = m_componentIDToNameLUT.find(typeID);
	if (l_result != m_componentIDToNameLUT.end())
	{
		return l_result->second.c_str();
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: Unknown ComponentTypeID: ", typeID);
		return nullptr;
	}
}

uint32_t InnoFileSystem::getComponentTypeID(const char* typeName)
{
	auto l_result = m_componentNameToIDLUT.find(typeName);
	if (l_result != m_componentNameToIDLUT.end())
	{
		return l_result->second;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: Unknown ComponentTypeName: ", typeName);
		return 0;
	}
}

std::string InnoFileSystem::getCurrentSceneName()
{
	auto l_currentSceneName = m_currentScene.substr(0, m_currentScene.find(".InnoScene"));
	l_currentSceneName = l_currentSceneName.substr(l_currentSceneName.rfind("//") + 2);
	return l_currentSceneName;
}

bool InnoFileSystem::loadScene(const char* fileName, bool AsyncLoad)
{
	if (m_currentScene == fileName)
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: Scene ", fileName, " has already loaded now.");
		return true;
	}

	if (AsyncLoad)
	{
		return prepareForLoadingScene(fileName);
	}
	else
	{
		m_nextLoadingScene = fileName;
		m_isLoadingScene = true;
		g_pModuleManager->getTaskSystem()->waitAllTasksToFinish();
		InnoFileSystemNS::loadScene(m_nextLoadingScene.c_str());
		GetComponentManager(VisibleComponent)->LoadAssetsForComponents(AsyncLoad);
		return true;
	}
}

bool InnoFileSystem::saveScene(const char* fileName)
{
	return InnoFileSystemNS::saveScene(fileName);
}

bool InnoFileSystem::isLoadingScene()
{
	return m_isLoadingScene;
}

bool InnoFileSystem::addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingStartCallbacks.emplace_back(functor, priority);
	return true;
}

bool InnoFileSystem::addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingFinishCallbacks.emplace_back(functor, priority);
	return true;
}

bool InnoFileSystem::addCPPClassFiles(const CPPClassDesc& desc)
{
	// Build header file
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(IOService::getWorkingDirectory() + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: std::ofstream: can't open file ", l_headerFileName.c_str(), "!");
		return false;
	}

	// Common headers include
	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"Common/InnoType.h\"" << std::endl;
	l_headerFile << "#include \"Common/InnoClassTemplate.h\"" << std::endl;
	l_headerFile << std::endl;

	// Abstraction type
	if (desc.isInterface)
	{
		l_headerFile << "class ";
	}
	else
	{
		l_headerFile << "class ";
	}

	l_headerFile << desc.className;

	// Inheriance type
	if (!desc.parentClass.empty())
	{
		l_headerFile << " : public " << desc.parentClass;
	}

	l_headerFile << std::endl;

	// Class decl body
	l_headerFile << "{" << std::endl;
	l_headerFile << "public:" << std::endl;

	// Ctor type
	if (desc.isInterface)
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_DEFALUT(" << desc.className << ");" << std::endl;
		}
	}
	else
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_DEFALUT(" << desc.className << ");" << std::endl;
		}
	}

	l_headerFile << std::endl;
	l_headerFile << "  bool setup();" << std::endl;
	l_headerFile << "  bool initialize();" << std::endl;
	l_headerFile << "  bool update();" << std::endl;
	l_headerFile << "  bool terminate();" << std::endl;
	l_headerFile << "  ObjectStatus getStatus();" << std::endl;

	l_headerFile << std::endl;
	l_headerFile << "private:" << std::endl;
	l_headerFile << "  ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;" << std::endl;
	l_headerFile << "};" << std::endl;

	l_headerFile.close();

	InnoLogger::Log(LogLevel::Success, "FileSystem: ", l_headerFileName.c_str(), " has been generated.");
	return true;
}