#include "inventory.h"

Inventory::Inventory(OpenGLContext *context, float x, float y, float width, float height)
    : Drawable(context), m_x(x), m_y(y), m_width(width), m_height(height), m_selected(0),
      m_num_blocks(int(m_width / slot_width) * int(m_height / slot_height)), m_num_col(int(m_width / slot_width)),
      m_inventory_open(false), m_picked_up(-1)
{

    // initial set of blocks in inventory
    std::array<BlockType, 10> blocks = { EMPTY, GRASS, DIRT, STONE, WATER, LAVA };

    // fills up the inventory
    for (int i = 0; i < blocks.size(); i++) {
        m_slots.push_back(blocks[i]);
//      m_blocks.push_back(new Block(context, blocks[i]));

    }
}



void Inventory::create() {
    std::vector<vertexAttribute> vboData;
    std::vector<GLuint> idxData;
    float tol = 1e-4;

    // changes depending on if the inventory is open or not
    float height = m_inventory_open ? m_height : slot_height;
    height += m_y - tol;

    float width = m_x + m_width - tol;

    // keeps track of the slot we are currently processing
    int slot = 0;

    for (float y = m_y; y < height; y += slot_height) {
        for (float x = m_x; x < width; x += slot_width) {
            std::array<vertexAttribute, 4> attrs;

            float col = slot == m_selected ? 0.6 : 1.f;

            for (int i = 0; i < offsets.size(); i++) {
                glm::ivec2 offset = offsets[i];
                attrs[i].pos = glm::vec4(x + offset.x*slot_width, y + offset.y*slot_height, 0.1f, 1.f);
                attrs[i].uv = glm::vec2(offset.x, offset.y);
                attrs[i].animFlag = col;
            }

            int slotIdx = slot * 4;
            idxData.push_back(slotIdx);
            idxData.push_back(slotIdx+1);
            idxData.push_back(slotIdx+2);

            idxData.push_back(slotIdx);
            idxData.push_back(slotIdx+2);
            idxData.push_back(slotIdx+3);

            slot++;
            vboData.insert(vboData.end(), attrs.data(), attrs.data() + 4);
        }
    }

    m_count = static_cast<int>(idxData.size());

    // send index data to the GPU
    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);

    // send VBO data to the GPU
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(vertexAttribute), vboData.data(), GL_STATIC_DRAW);
}


void Inventory::draw(ShaderProgram *slot_prog, Texture &slotTexture, ShaderProgram *block_prog)
{
    mp_context->glDisable((GL_DEPTH_TEST));

    slot_prog->setModelMatrix(glm::mat4());

    // bind the texture data to the appropriate slot
    slotTexture.bind(INVENTORY_SLOT_TEXTURE_SLOT);
    slot_prog->setInventorySlotTextureSampler(INVENTORY_SLOT_TEXTURE_SLOT);

    // draw the inventory with the appropriate
    slot_prog->draw(*this, true);
    mp_context->glEnable((GL_DEPTH_TEST));
}

// returns true if coords (x,y) are within the bounds of the inventory
bool Inventory::is_within_bounds(float x, float y) {
    return x >= m_x && y >= m_y && x <= m_x+m_width && y <= m_y+m_height;
}

void Inventory::select_left() {}
void Inventory::select_right() {}




