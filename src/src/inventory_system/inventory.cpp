#include "inventory.h"

Inventory::Inventory(OpenGLContext *context, float x, float y, float width, float height)
    : Drawable(context), m_x(x), m_y(y), m_width(width), m_height(height), m_selected(0),
      m_num_blocks(int(m_width / slot_width) * int(m_height / slot_height)), m_num_col(int(m_width / slot_width)),
      m_inventory_active(false), m_picked_up(-1)
{

    // initial set of blocks in inventory
    std::array<BlockType, 10> blocks = { EMPTY, GRASS, DIRT, STONE, WATER, LAVA };

    // fills up the inventory
    for (int i = 0; i < blocks.size(); i++) {
        m_slots.push_back(blocks[i]);
//      m_blocks.push_back(new Block(context, blocks[i]));

    }
}
