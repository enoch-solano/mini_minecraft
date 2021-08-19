#ifndef INVENTORY_H
#define INVENTORY_H

#include "drawable.h"
#include "block.h"
#include "glm_includes.h"

#include "../scene/chunk.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>


class Inventory : public Drawable {
    bool m_inventory_open;

    std::vector<BlockType> m_slots;
    std::vector<Block*> m_blocks;

    int m_selected;     // index of selected block
    int m_picked_up;

    float m_x, m_y, m_width, m_height;

    float slot_width  = 0.25 * 3.f / 4.f;
    float slot_height = 0.25;

    std::vector<glm::ivec2> offsets = { glm::ivec2(0, 0),
                                        glm::ivec2(1, 0),
                                        glm::ivec2(1, 1),
                                        glm::ivec2(0, 1) };

    int m_num_blocks;
    int m_num_col;

public:
    Inventory(OpenGLContext *context, float x, float y, float width, float height);

    // returns true if coords (x,y) are within the bounds of the inventory
    bool is_within_bounds(float x, float y);

    // helper functions for interacting with inventory system
    BlockType pick_up_block(float x, float y);
    BlockType drop_block(float x, float y, BlockType block);
    void reset_picked_up();

    void select_block(float x, float y);
    BlockType remove_selected_block();

    int get_index(float x, float y);

    void select_left();
    void select_right();

    void select_horizontal(int key) {
        key == Qt::Key_Left ? select_left() : select_right();
    }

    void select_up();
    void select_down();

    bool add_block(BlockType block);

    virtual ~Inventory() {}
    virtual void create() override;
    virtual void toggle_active_mode() {}
    virtual void draw(ShaderProgram *slot_prog, Texture &slotTexture, ShaderProgram *block_prog);
};

#endif // INVENTORY_H
