////////////////////////////////////////////////////////////////////////////////
//
// The University of Illinois/NCSA
// Open Source License (NCSA)
// 
// Copyright (c) 2014-2015, Advanced Micro Devices, Inc. All rights reserved.
// 
// Developed by:
// 
//                 AMD Research and AMD HSA Software Development
// 
//                 Advanced Micro Devices, Inc.
// 
//                 www.amd.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//  - Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimers.
//  - Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimers in
//    the documentation and/or other materials provided with the distribution.
//  - Neither the names of Advanced Micro Devices, Inc,
//    nor the names of its contributors may be used to endorse or promote
//    products derived from this Software without specific prior written
//    permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS WITH THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "core/inc/amd_memory_region.h"

#include <algorithm>

#include "core/inc/runtime.h"
#include "core/inc/amd_cpu_agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/util/utils.h"

namespace amd {
void* MemoryRegion::AllocateKfdMemory(const HsaMemFlags& flag,
                                      HSAuint32 node_id, size_t size) {
  void* ret = NULL;
  const HSAKMT_STATUS status = hsaKmtAllocMemory(node_id, size, flag, &ret);
  return (status == HSAKMT_STATUS_SUCCESS) ? ret : NULL;
}

void MemoryRegion::FreeKfdMemory(void* ptr, size_t size) {
  if (ptr == NULL || size == 0) {
    return;
  }

  HSAKMT_STATUS status = hsaKmtFreeMemory(ptr, size);
  assert(status == HSAKMT_STATUS_SUCCESS);
}

bool MemoryRegion::RegisterMemory(void* ptr, size_t size, size_t num_nodes,
                                  const uint32_t* nodes) {
  assert(ptr != NULL);
  assert(size != 0);
  assert(num_nodes != 0);
  assert(nodes != NULL);

  const HSAKMT_STATUS status = hsaKmtRegisterMemoryToNodes(
      ptr, size, num_nodes, const_cast<uint32_t*>(nodes));
  return (status == HSAKMT_STATUS_SUCCESS);
}

void MemoryRegion::DeregisterMemory(void* ptr) { hsaKmtDeregisterMemory(ptr); }
<<<<<<< HEAD

bool MemoryRegion::MakeKfdMemoryResident(size_t num_node, const uint32_t* nodes,
                                         void* ptr, size_t size,
                                         uint64_t* alternate_va,
                                         HsaMemMapFlags map_flag) {
  assert(num_node > 0);
  assert(nodes != NULL);

  *alternate_va = 0;
  const HSAKMT_STATUS status =
      hsaKmtMapMemoryToGPUNodes(ptr, size, alternate_va, map_flag, num_node,
                                const_cast<uint32_t*>(nodes));
=======

bool MemoryRegion::MakeKfdMemoryResident(size_t num_node, const uint32_t* nodes,
                                         void* ptr, size_t size,
                                         uint64_t* alternate_va,
                                         HsaMemMapFlags map_flag) {
  assert(num_node > 0);
  assert(nodes != NULL);

  // TODO(bwicakso): hsaKmtMapMemoryToGPUNodes is currently broken.
  *alternate_va = 0;
  const HSAKMT_STATUS status = hsaKmtMapMemoryToGPU(ptr, size, alternate_va);
      //hsaKmtMapMemoryToGPUNodes(ptr, size, alternate_va, map_flag, num_node,
      //                          const_cast<uint32_t*>(nodes));
>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f

  return (status == HSAKMT_STATUS_SUCCESS);
}

void MemoryRegion::MakeKfdMemoryUnresident(void* ptr) {
  hsaKmtUnmapMemoryToGPU(ptr);
}

MemoryRegion::MemoryRegion(bool fine_grain, bool full_profile, uint32_t node_id,
                           const HsaMemoryProperties& mem_props)
    : core::MemoryRegion(fine_grain, full_profile),
      node_id_(node_id),
      owner_(NULL),
      mem_props_(mem_props),
      max_single_alloc_size_(0),
      virtual_size_(0) {
  virtual_size_ = GetPhysicalSize();

  mem_flag_.Value = 0;
  map_flag_.Value = 0;

  static const HSAuint64 kGpuVmSize = (1ULL << 40);

  if (IsLocalMemory()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess =
        (mem_props_.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE) ? 0 : 1;
    mem_flag_.ui32.NonPaged = 1;

    map_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;

    virtual_size_ = kGpuVmSize;
  } else if (IsSystem()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = 1;
    mem_flag_.ui32.CachePolicy = HSA_CACHING_CACHED;

    map_flag_.ui32.HostAccess = 1;
    map_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;

    virtual_size_ =
        (full_profile) ? os::GetUserModeVirtualMemorySize() : kGpuVmSize;
  }

  max_single_alloc_size_ =
      AlignDown(static_cast<size_t>(GetPhysicalSize()), kPageSize_);

  mem_flag_.ui32.CoarseGrain = (fine_grain) ? 0 : 1;

  assert(GetVirtualSize() != 0);
  assert(GetPhysicalSize() <= GetVirtualSize());
  assert(IsMultipleOf(max_single_alloc_size_, kPageSize_));
}

MemoryRegion::~MemoryRegion() {}

hsa_status_t MemoryRegion::Allocate(size_t size, void** address) const {
  return Allocate(false, size, address);
}

hsa_status_t MemoryRegion::Allocate(bool restrict_access, size_t size,
                                    void** address) const {
  if (address == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  if (!IsSystem() && !IsLocalMemory()) {
    return HSA_STATUS_ERROR_INVALID_ALLOCATION;
  }

  if (size > max_single_alloc_size_) {
    return HSA_STATUS_ERROR_INVALID_ALLOCATION;
  }

  size = AlignUp(size, kPageSize_);

  *address = AllocateKfdMemory(mem_flag_, node_id_, size);

  if (*address != NULL) {
<<<<<<< HEAD
=======
    // Register all possible GPU that may access the memory.
    // TODO(bwicakso): remove HSA profile check when KFD support memory
    // registration on APU.
    if (!full_profile() &&
        core::Runtime::runtime_singleton_->gpu_ids().size() > 0) {
      if (!RegisterMemory(*address, size,
                          core::Runtime::runtime_singleton_->gpu_ids().size(),
                          &core::Runtime::runtime_singleton_->gpu_ids()[0])) {
        return HSA_STATUS_ERROR;
      }
    }

>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f
    // Commit the memory.
    // For system memory, on non-restricted allocation, map it to all GPUs. On
    // restricted allocation, only CPU is allowed to access by default, so 
    // no need to map
    // For local memory, only map it to the owning GPU. Mapping to other GPU,
    // if the access is allowed, is performed on AllowAccess.
    HsaMemMapFlags map_flag = map_flag_;
    size_t map_node_count = 1;
    const uint32_t* map_node_id = &node_id_;

    if (IsSystem()) {
      if (!restrict_access) {
        // Map to all GPU agents.
        map_node_count = core::Runtime::runtime_singleton_->gpu_ids().size();

        if (map_node_count == 0) {
          // No need to pin since no GPU in the platform.
          return HSA_STATUS_SUCCESS;
        }

        map_node_id = &core::Runtime::runtime_singleton_->gpu_ids()[0];
      } else {
        // No need to pin it for CPU exclusive access.
        return HSA_STATUS_SUCCESS;
      }
    }

    uint64_t alternate_va = 0;
    const bool is_resident = MakeKfdMemoryResident(
        map_node_count, map_node_id, *address, size, &alternate_va, map_flag);

    const bool require_pinning =
        (!full_profile() || IsLocalMemory() || IsScratch());

    if (require_pinning && !is_resident) {
<<<<<<< HEAD
=======
      DeregisterMemory(*address);
>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f
      FreeKfdMemory(*address, size);
      *address = NULL;
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    }

    return HSA_STATUS_SUCCESS;
  }

  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

hsa_status_t MemoryRegion::Free(void* address, size_t size) const {
  MakeKfdMemoryUnresident(address);

  FreeKfdMemory(address, size);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::GetInfo(hsa_region_info_t attribute,
                                   void* value) const {
  switch (attribute) {
    case HSA_REGION_INFO_SEGMENT:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((hsa_region_segment_t*)value) = HSA_REGION_SEGMENT_GLOBAL;
          break;
        case HSA_HEAPTYPE_GPU_LDS:
          *((hsa_region_segment_t*)value) = HSA_REGION_SEGMENT_GROUP;
          break;
        default:
          assert(false && "Memory region should only be global, group");
          break;
      }
      break;
    case HSA_REGION_INFO_GLOBAL_FLAGS:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
          *((uint32_t*)value) = fine_grain()
                                    ? (HSA_REGION_GLOBAL_FLAG_KERNARG |
                                       HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)
                                    : HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED;
          break;
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((uint32_t*)value) = HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED;
          break;
        default:
          *((uint32_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_SIZE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = static_cast<size_t>(GetPhysicalSize());
          break;
        default:
          *((size_t*)value) = static_cast<size_t>(
              (full_profile()) ? GetVirtualSize() : GetPhysicalSize());
          break;
      }
      break;
    case HSA_REGION_INFO_ALLOC_MAX_SIZE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
        case HSA_HEAPTYPE_SYSTEM:
          *((size_t*)value) = max_single_alloc_size_;
          break;
        default:
          *((size_t*)value) = 0;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_ALLOWED:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((bool*)value) = true;
          break;
        default:
          *((bool*)value) = false;
          break;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_GRANULE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = kPageSize_;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_ALIGNMENT:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = kPageSize_;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    default:
      switch ((hsa_amd_region_info_t)attribute) {
        case HSA_AMD_REGION_INFO_HOST_ACCESSIBLE:
          *((bool*)value) =
              (mem_props_.HeapType == HSA_HEAPTYPE_SYSTEM) ? true : false;
          break;
        case HSA_AMD_REGION_INFO_BASE:
          *((void**)value) = reinterpret_cast<void*>(GetBaseAddress());
          break;
        case HSA_AMD_REGION_INFO_BUS_WIDTH:
          *((uint32_t*)value) = BusWidth();
          break;
        case HSA_AMD_REGION_INFO_MAX_CLOCK_FREQUENCY:
          *((uint32_t*)value) = MaxMemCloc();
          break;
        default:
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
          break;
      }
      break;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::GetPoolInfo(hsa_amd_memory_pool_info_t attribute,
                                       void* value) const {
  switch (attribute) {
    case HSA_AMD_MEMORY_POOL_INFO_SEGMENT:
    case HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS:
    case HSA_AMD_MEMORY_POOL_INFO_SIZE:
    case HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED:
    case HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_GRANULE:
    case HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALIGNMENT:
      return GetInfo(static_cast<hsa_region_info_t>(attribute), value);
      break;
    case HSA_AMD_MEMORY_POOL_INFO_ACCESSIBLE_BY_ALL:
      *((bool*)value) = IsSystem() ? true : false;
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::GetAgentPoolInfo(
    const core::Agent& agent, hsa_amd_agent_memory_pool_info_t attribute,
    void* value) const {
<<<<<<< HEAD
  const uint32_t node_id_from =
      (agent.device_type() == core::Agent::kAmdCpuDevice)
          ? reinterpret_cast<const amd::CpuAgent&>(agent).node_id()
          : reinterpret_cast<const amd::GpuAgentInt&>(agent).node_id();

  const uint32_t node_id_to =
      (owner_->device_type() == core::Agent::kAmdCpuDevice)
          ? reinterpret_cast<const amd::CpuAgent*>(owner_)->node_id()
          : reinterpret_cast<const amd::GpuAgentInt*>(owner_)->node_id();

  const core::Runtime::LinkInfo link_info =
      core::Runtime::runtime_singleton_->GetLinkInfo(node_id_from, node_id_to);

  switch (attribute) {
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS:
      /**
      *  ---------------------------------------------------
      *  |              |CPU        |GPU (owner)|GPU (peer) |
      *  ---------------------------------------------------
      *  |system memory |allowed    |disallowed |disallowed |
      *  ---------------------------------------------------
      *  |fb private    |never      |allowed    |never      |
      *  ---------------------------------------------------
      *  |fb public     |disallowed |allowed    |disallowed |
      *  ---------------------------------------------------
      *  |others        |never      |allowed    |never      |
      *  ---------------------------------------------------
      */
      *((hsa_amd_memory_pool_access_t*)value) =
          (((IsSystem()) &&
            (agent.device_type() == core::Agent::kAmdCpuDevice)) ||
           (&agent == owner_))
              ? HSA_AMD_MEMORY_POOL_ACCESS_ALLOWED_BY_DEFAULT
              : (IsSystem() || (IsPublic() && link_info.num_hop > 0))
                    ? HSA_AMD_MEMORY_POOL_ACCESS_DISALLOWED_BY_DEFAULT
                    : HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED;
      break;
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_NUM_LINK_HOPS:
      *((uint32_t*)value) = link_info.num_hop;
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_LINK_INFO:
      memset(value, 0, sizeof(hsa_amd_memory_pool_link_info_t));
      if (link_info.num_hop > 0) {
        memcpy(value, &link_info.info, sizeof(hsa_amd_memory_pool_link_info_t));
      }
=======
  switch (attribute) {
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS:
      *((hsa_amd_memory_pool_access_t*)value) = GetPoolAccessType(agent);
      break;
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_NUM_LINK_HOPS:
      *((uint32_t*)value) = 1;  // TODO(bwicakso): more info needed from kfd.
      break;
    case HSA_AMD_AGENT_MEMORY_POOL_INFO_LINK_INFO:
      // TODO(bwicakso): more info needed from kfd.
      memset(value, 0, sizeof(hsa_amd_memory_pool_link_info_t));
>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return HSA_STATUS_SUCCESS;
}

<<<<<<< HEAD
=======
hsa_amd_memory_pool_access_t MemoryRegion::GetPoolAccessType(
    const core::Agent& agent) const {
  /**
   *  ---------------------------------------------------
   *  |              |CPU        |GPU (owner)|GPU (peer) |
   *  ---------------------------------------------------
   *  |system memory |allowed    |disallowed |disallowed |
   *  ---------------------------------------------------
   *  |fb private    |never      |allowed    |never      |
   *  ---------------------------------------------------
   *  |fb public     |disallowed |allowed    |disallowed |
   *  ---------------------------------------------------
   *  |others        |never      |allowed    |never      |
   *  ---------------------------------------------------
   */
  return (((IsSystem()) &&
           (agent.device_type() == core::Agent::kAmdCpuDevice)) ||
          (&agent == owner_))
             ? HSA_AMD_MEMORY_POOL_ACCESS_ALLOWED_BY_DEFAULT
             : (IsSystem() || IsPublic())
                   ? HSA_AMD_MEMORY_POOL_ACCESS_DISALLOWED_BY_DEFAULT
                   : HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED;
}

>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f
hsa_status_t MemoryRegion::AllowAccess(uint32_t num_agents,
                                       const hsa_agent_t* agents,
                                       const void* ptr, size_t size) const {
  if (num_agents == 0 || agents == NULL || ptr == NULL || size == 0) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  if (!IsSystem() && !IsLocalMemory()) {
    return HSA_STATUS_ERROR;
  }

  bool cpu_in_list = false;

  std::vector<uint32_t> whitelist_nodes;
  for (uint32_t i = 0; i < num_agents; ++i) {
    const core::Agent* agent = core::Agent::Convert(agents[i]);
    if (agent == NULL || !agent->IsValid()) {
      return HSA_STATUS_ERROR_INVALID_AGENT;
    }

    if (agent->device_type() == core::Agent::kAmdGpuDevice) {
      whitelist_nodes.push_back(
          reinterpret_cast<const amd::GpuAgentInt*>(agent)->node_id());
    } else {
      cpu_in_list = true;
    }
  }

  if (whitelist_nodes.size() == 0 && IsSystem()) {
    assert(cpu_in_list);
    // This is a system region and only CPU agents in the whitelist.
    // No need to call map.
    return HSA_STATUS_SUCCESS;
  }

  // If this is a local memory region, the owning gpu always needs to be in
  // the whitelist.
  if (IsPublic() &&
      std::find(whitelist_nodes.begin(), whitelist_nodes.end(), node_id_) ==
          whitelist_nodes.end()) {
    whitelist_nodes.push_back(node_id_);
  }

  HsaMemMapFlags map_flag = map_flag_;
  map_flag.ui32.HostAccess |= (cpu_in_list) ? 1 : 0;

  uint64_t alternate_va = 0;
  return (amd::MemoryRegion::MakeKfdMemoryResident(
             whitelist_nodes.size(), &whitelist_nodes[0],
             const_cast<void*>(ptr), size, &alternate_va, map_flag))
             ? HSA_STATUS_SUCCESS
             : HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}
<<<<<<< HEAD

hsa_status_t MemoryRegion::CanMigrate(const MemoryRegion& dst,
                                      bool& result) const {
  // TODO(bwicakso): not implemented yet.
  result = false;
  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

hsa_status_t MemoryRegion::Migrate(uint32_t flag, const void* ptr) const {
  // TODO(bwicakso): not implemented yet.
  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

=======

hsa_status_t MemoryRegion::CanMigrate(const MemoryRegion& dst,
                                      bool& result) const {
  // TODO(bwicakso): not implemented yet.
  result = false;
  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

hsa_status_t MemoryRegion::Migrate(uint32_t flag, const void* ptr) const {
  // TODO(bwicakso): not implemented yet.
  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

>>>>>>> 85ad07b87d1513e094d206ed8d5f49946f86991f
hsa_status_t MemoryRegion::Lock(uint32_t num_agents, const hsa_agent_t* agents,
                                void* host_ptr, size_t size,
                                void** agent_ptr) const {
  if (!IsSystem()) {
    return HSA_STATUS_ERROR;
  }

  if (full_profile()) {
    // For APU, any host pointer is always accessible by the gpu.
    *agent_ptr = host_ptr;
    return HSA_STATUS_SUCCESS;
  }

  std::vector<HSAuint32> whitelist_nodes;
  if (num_agents == 0 || agents == NULL) {
    // Map to all GPU agents.
    whitelist_nodes = core::Runtime::runtime_singleton_->gpu_ids();
  } else {
    for (int i = 0; i < num_agents; ++i) {
      core::Agent* agent = core::Agent::Convert(agents[i]);
      if (agent == NULL || !agent->IsValid()) {
        return HSA_STATUS_ERROR_INVALID_AGENT;
      }

      if (agent->device_type() == core::Agent::kAmdGpuDevice) {
        whitelist_nodes.push_back(
            reinterpret_cast<amd::GpuAgentInt*>(agent)->node_id());
      }
    }
  }

  if (whitelist_nodes.size() == 0) {
    // No GPU agents in the whitelist. So no need to register and map since the
    // platform only has CPUs.
    *agent_ptr = host_ptr;
    return HSA_STATUS_SUCCESS;
  }

  // Call kernel driver to register and pin the memory.

  // Adjust the address and size to be cacheline aligned to satisfy the
  // requirement from kernel driver.
  static const size_t kCacheAlignment = 64;
  const uintptr_t cache_offset =
      reinterpret_cast<uintptr_t>(host_ptr) & (kCacheAlignment - 1);
  host_ptr =
      reinterpret_cast<void*>(reinterpret_cast<char*>(host_ptr) - cache_offset);
  size = AlignUp((cache_offset + size), kCacheAlignment);

  if (RegisterMemory(host_ptr, size, whitelist_nodes.size(),
                     &whitelist_nodes[0])) {
    uint64_t alternate_va = 0;
    if (MakeKfdMemoryResident(whitelist_nodes.size(), &whitelist_nodes[0],
                              host_ptr, size, &alternate_va, map_flag_)) {
      assert(alternate_va != 0);
      // Adjust the offset of the agent ptr in case host ptr is not cacheline
      // aligned.
      *agent_ptr = reinterpret_cast<void*>(alternate_va + cache_offset);
      return HSA_STATUS_SUCCESS;
    }
    amd::MemoryRegion::DeregisterMemory(host_ptr);
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  return HSA_STATUS_ERROR;
}

hsa_status_t MemoryRegion::Unlock(void* host_ptr) const {
  if (!IsSystem()) {
    return HSA_STATUS_ERROR;
  }

  if (full_profile()) {
    return HSA_STATUS_SUCCESS;
  }

  static const size_t kCacheAlignment = 64;
  host_ptr = AlignDown(host_ptr, kCacheAlignment);

  MakeKfdMemoryUnresident(host_ptr);
  DeregisterMemory(host_ptr);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::AssignAgent(void* ptr, size_t size,
                                       const core::Agent& agent,
                                       hsa_access_permission_t access) const {
  return HSA_STATUS_SUCCESS;
}

}  // namespace
