#include "inventory.h"
#include <iostream>

#define NUM_BLOCKS 7

Inventory::Inventory(OpenGLContext *context, float x, float y, float width, float height)
    : Drawable(context), m_x(x), m_y(y), m_width(width), m_height(height), m_selected(0),
      m_num_blocks(int(m_width / slot_width) * int(m_height / slot_height)), m_num_col(int(m_width / slot_width)),
      m_inventory_open(false), m_picked_up(-1)
{

    // initial set of blocks in inventory
    std::array<BlockType, NUM_BLOCKS> blocks = { GRASS, DIRT, STONE, SAND, SPONGE, RED_CLAY, SNOW };

    // fills up the inventory
    for (int i = 0; i < INVENTORY_NUM_SLOTS; i++) {

        BlockType block = i < NUM_BLOCKS ? blocks[i] : EMPTY;
        m_blocks[i] = new Block(context, block);
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

    for (int i = 0; i < m_num_blocks; i++) {
        m_blocks[i]->destroy();
        m_blocks[i]->create();
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


void Inventory::draw(ShaderProgram *slot_prog, Texture &slotTexture, ShaderProgram *block_prog, Texture &blockTexture)
{
    mp_context->glDisable((GL_DEPTH_TEST));

    slot_prog->setModelMatrix(glm::mat4());

    // bind the texture data to the appropriate slot
    slotTexture.bind(INVENTORY_SLOT_TEXTURE_SLOT);
    slot_prog->setInventorySlotTextureSampler(INVENTORY_SLOT_TEXTURE_SLOT);

    // draw the inventory
    slot_prog->draw(*this, true);

    mp_context->glEnable((GL_DEPTH_TEST));
    mp_context->glDisable((GL_DEPTH_TEST));


    // bind the texture data to the appropriate slot
    blockTexture.bind(MINECRAFT_BLOCK_TEXTURE_SLOT);
    block_prog->setBlockTextureSampler(MINECRAFT_BLOCK_TEXTURE_SLOT);

    // variables for drawing the blocks in the slots
    float x0 = m_x + slot_width/2.f;
    float y0 = m_y + slot_height/2.f;

    int num_col = int(m_width / slot_width);
    int num_blocks = m_inventory_open ? m_num_blocks : num_col;

    // draw the blocks in the slots
    for (int i = 0; i < num_blocks; i++) {
        if (m_blocks[i]->get_type() == EMPTY) {
            continue;
        }

        float x = x0 + slot_width  * (i % num_col);
        float y = y0 + slot_height * (i / num_col);

        block_prog->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, y, 0)));
        block_prog->draw(*(m_blocks[i]), true);
    }

    mp_context->glEnable((GL_DEPTH_TEST));
}

// returns true if coords (x,y) are within the bounds of the inventory
bool Inventory::is_within_bounds(float x, float y) {
    return x >= m_x && y >= m_y && x <= m_x+m_width && y <= m_y+m_height;
}

int Inventory::get_index(float x, float y) {
    int row = int(std::floor((y - m_y) / slot_height));
    int col = int(std::floor((x - m_x) / slot_width));

    return col + row*m_num_col;
}

void Inventory::select_block(float x, float y) {
    m_selected = get_index(x, y);
}

void Inventory::select_left() {
    m_selected--;
    m_selected = m_inventory_open ? (m_selected + m_num_blocks) % m_num_blocks : (m_selected + m_num_col) % m_num_col;
}

void Inventory::select_right() {
    m_selected++;
    m_selected = m_inventory_open ? m_selected % m_num_blocks : m_selected % m_num_col;
}

void Inventory::select_horizontal(int key) {
    key == Qt::Key_Left ? select_left() : select_right();
}

void Inventory::select_down() {
    m_selected -= m_num_col;
    m_selected += m_num_blocks;
    m_selected %= m_num_blocks;
}

void Inventory::select_up() {
    m_selected += m_num_col;
    m_selected %= m_num_blocks;
}

void Inventory::select_vertical(int key) {
    key == Qt::Key_Down ? select_down() : select_up();
}

void Inventory::toggle_mode(bool open) {
    m_inventory_open = open;

    if (!m_inventory_open) {
        m_selected = 0;
    }
}



