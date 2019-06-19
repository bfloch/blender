#pragma once

#include "BLI_array_ref.hpp"
#include "BLI_small_vector.hpp"
#include "BLI_small_set_vector.hpp"
#include "BLI_math.hpp"
#include "BLI_string_ref.hpp"
#include "BLI_small_map.hpp"

#include "core.hpp"

namespace BParticles {

using BLI::ArrayRef;
using BLI::float3;
using BLI::SmallMap;
using BLI::SmallSet;
using BLI::SmallSetVector;
using BLI::SmallVector;
using BLI::StringRef;

class ParticlesContainer;
class ParticlesBlock;

class ParticlesContainer {
 private:
  AttributesInfo m_attributes;
  SmallSet<ParticlesBlock *> m_blocks;
  uint m_block_size;

 public:
  ParticlesContainer(AttributesInfo &attributes, uint block_size);

  ~ParticlesContainer();

  uint block_size() const;

  AttributesInfo &attributes();

  const SmallSet<ParticlesBlock *> &active_blocks();

  ParticlesBlock *new_block();
  void release_block(ParticlesBlock *block);
};

class ParticlesBlock {
  ParticlesContainer &m_container;
  AttributeArraysCore m_arrays;
  uint m_active_amount;

 public:
  ParticlesBlock(ParticlesContainer &container);

  uint &active_amount();
  uint inactive_amount();
  bool is_full();
  bool is_empty();
  uint next_inactive_index();
  uint size();

  ParticlesContainer &container();

  void clear();

  AttributeArraysCore &arrays_core();
  AttributeArrays slice(uint start, uint length);
  AttributeArrays slice_all();
  AttributeArrays slice_active();

  void move(uint old_index, uint new_index);

  static void MoveUntilFull(ParticlesBlock &from, ParticlesBlock &to);
  static void Compress(ArrayRef<ParticlesBlock *> blocks);
};

/* Particles Container
 ***********************************************/

inline uint ParticlesContainer::block_size() const
{
  return m_block_size;
}

inline AttributesInfo &ParticlesContainer::attributes()
{
  return m_attributes;
}

inline const SmallSet<ParticlesBlock *> &ParticlesContainer::active_blocks()
{
  return m_blocks;
}

/* Particles Block
 ****************************************/

inline uint &ParticlesBlock::active_amount()
{
  return m_active_amount;
}

inline uint ParticlesBlock::inactive_amount()
{
  return this->size() - m_active_amount;
}

inline bool ParticlesBlock::is_full()
{
  return m_active_amount == this->size();
}

inline bool ParticlesBlock::is_empty()
{
  return m_active_amount == 0;
}

inline uint ParticlesBlock::next_inactive_index()
{
  return m_active_amount;
}

inline uint ParticlesBlock::size()
{
  return m_container.block_size();
}

inline void ParticlesBlock::clear()
{
  m_active_amount = 0;
}

inline ParticlesContainer &ParticlesBlock::container()
{
  return m_container;
}

inline AttributeArrays ParticlesBlock::slice(uint start, uint length)
{
  return m_arrays.slice_all().slice(start, length);
}

inline AttributeArrays ParticlesBlock::slice_all()
{
  return m_arrays.slice_all();
}

inline AttributeArrays ParticlesBlock::slice_active()
{
  return this->slice(0, m_active_amount);
}

inline AttributeArraysCore &ParticlesBlock::arrays_core()
{
  return m_arrays;
}

inline void ParticlesBlock::move(uint old_index, uint new_index)
{
  AttributesInfo &attributes = m_container.attributes();
  AttributeArrays arrays = this->slice_all();

  for (uint i : attributes.byte_attributes()) {
    auto buffer = arrays.get_byte(i);
    buffer[new_index] = buffer[old_index];
  }
  for (uint i : attributes.float_attributes()) {
    auto buffer = arrays.get_float(i);
    buffer[new_index] = buffer[old_index];
  }
  for (uint i : attributes.float3_attributes()) {
    auto buffer = arrays.get_float3(i);
    buffer[new_index] = buffer[old_index];
  }
}

}  // namespace BParticles