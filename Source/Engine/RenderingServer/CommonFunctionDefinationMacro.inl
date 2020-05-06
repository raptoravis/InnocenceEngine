#define AddComponent(renderingServer, component) \
component##Component * renderingServer##RenderingServer::Add##component##Component(const char * name) \
{ \
static std::atomic<uint32_t> l_count = 0; \
l_count++; \
std::string l_name; \
if (strcmp(name, "")) \
{ \
	l_name = name; \
} \
else \
{ \
	l_name = (std::string(#component) + "_" + std::to_string(l_count) + "/"); \
} \
auto l_rawPtr = m_##component##ComponentPool->Spawn(); \
auto l_result = new(l_rawPtr)renderingServer##component##Component(); \
l_result->m_UUID = InnoRandomizer::GenerateUUID(); \
l_result->m_ObjectStatus = ObjectStatus::Created; \
l_result->m_ObjectSource = ObjectSource::Runtime; \
l_result->m_ObjectOwnership = ObjectOwnership::Engine; \
auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, l_name.c_str()); \
l_result->m_ParentEntity = l_parentEntity; \
l_result->m_ComponentType = g_pModuleManager->getFileSystem()->getComponentTypeID(#component"Component"); \
l_result->m_Name = l_name.c_str(); \
 \
return l_result; \
}
