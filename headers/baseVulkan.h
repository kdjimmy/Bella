#include <vulkan/vulkan.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <functional>
#include <cstring>
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct vk_Buffer
{
    VkBuffer m_buffer;
    size_t bufferStride = 0;
    uint32_t vertexCount = 0;
    VkDeviceAddress* device_Address = nullptr;
    VkDeviceMemory m_memory;
    size_t offset = 0;  
};


class vulkanBase
{
    public:
        VkDevice m_device;
        VkInstance m_instance;
        VkPhysicalDevice m_physicalDevice;
        //需要绑定memory
        template<typename T>
        VkBuffer createBuffer(VkBufferUsageFlags flags, VkSharingMode mode, size_t count) {
            VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            createInfo.size = sizeof(T) * count;
            createInfo.usage = flags;
            createInfo.sharingMode = mode;
            VkBuffer buffer;
            vkCreateBuffer(m_device, &createInfo, nullptr, &buffer);
            return buffer;
        }

        VkAccelerationStructureKHR createBlasStructure(const vk_Buffer& vertex_buffer, const vk_Buffer& index_buffer, uint32_t primitiveCount, 
            uint32_t primitiveOffset, uint32_t firstIndex, uint32_t transformOffset, uint32_t geometryCount)
        {
            VkAccelerationStructureGeometryKHR geometry{};
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData.deviceAddress = *vertex_buffer.device_Address;
            geometry.geometry.triangles.vertexStride = vertex_buffer.bufferStride;
            geometry.geometry.triangles.maxVertex = vertex_buffer.vertexCount;
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData.deviceAddress = *index_buffer.device_Address;

            VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
            rangeInfo.primitiveCount = primitiveCount;
            rangeInfo.primitiveOffset = primitiveOffset;
            rangeInfo.firstVertex = firstIndex;
            rangeInfo.transformOffset = transformOffset;

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
            buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buildInfo.geometryCount = geometryCount;
            buildInfo.pGeometries = &geometry;

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizeInfo);
        

            VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
            asCreateInfo.buffer = std::move(createBuffer<char>(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_SHARING_MODE_EXCLUSIVE, sizeInfo.accelerationStructureSize));
            asCreateInfo.size = sizeInfo.accelerationStructureSize;
            asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            VkAccelerationStructureKHR blas;
            vkCreateAccelerationStructureKHR(m_device, &asCreateInfo, nullptr, &blas);
            return blas;
        }

/*
        std::array<VkAccelerationStructureInstanceKHR, 2> instances{};
        // 茶壶实例
        instances[0].transform = {/* 4x3};
        instances[0].instanceCustomIndex = 0; // 着色器里可用
        instances[0].mask = 0xFF;
        instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[0].instanceShaderBindingTableRecordOffset = 0;
        instances[0].accelerationStructureReference = blasTeapotAddress;
        instancebuffer is a device buffer records these info
*/

        VkAccelerationStructureKHR createTlasStructure(VkTransformMatrixKHR &transformMatrix, uint32_t tableRecordOffset, const vk_Buffer& blasBuffer)
        {
            VkAccelerationStructureInstanceKHR instance{};
            instance.transform = transformMatrix;
            instance.mask = 0xFF;
            instance.instanceCustomIndex = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.instanceShaderBindingTableRecordOffset = tableRecordOffset;
            instance.accelerationStructureReference = *(blasBuffer.device_Address);
            VkBuffer instanceBuffer = createBuffer<char>(
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                VK_SHARING_MODE_EXCLUSIVE, sizeof(instance));
            void *data;
            vkMapMemory(m_device, instanceMemory, 0, VK_WHOLE_SIZE, 0, &data);
            memcpy(data, &instance, sizeof(instance));
            vkUnmapMemory(m_device, instanceMemory);
            VkBufferDeviceAddressInfoKHR addrInfo{};
            addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addrInfo.buffer = instanceBuffer;
            VkDeviceAddress address = vkGetBufferDeviceAddress(m_device, &addrInfo);

            VkAccelerationStructureGeometryKHR geometry{};
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            geometry.geometry.instances.sType = 
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            geometry.geometry.instances.arrayOfPointers = VK_FALSE;
            geometry.geometry.instances.data.deviceAddress = address;

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
            buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;

            uint32_t primitiveCount = 1; // 一个实例
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                    &buildInfo, &primitiveCount, &sizeInfo);

            // 4. 创建 TLAS buffer
            VkBuffer tlasBuffer = createBuffer<char>(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_SHARING_MODE_EXCLUSIVE, sizeInfo.accelerationStructureSize);

            // 5. 创建 TLAS 对象
            VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
            asCreateInfo.buffer = tlasBuffer;
            asCreateInfo.size   = sizeInfo.accelerationStructureSize;
            asCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            VkAccelerationStructureKHR tlas;
            vkCreateAccelerationStructureKHR(m_device, &asCreateInfo, nullptr, &tlas);

            return tlas;
        }
};