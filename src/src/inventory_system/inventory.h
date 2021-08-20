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

    // helper functions for interacting with inventory system
    BlockType pick_up_block(float x, float y);
    BlockType drop_block(float x, float y, BlockType block);
    void reset_picked_up();

    BlockType remove_selected_block();

    bool add_block(BlockType block);

    // used to change the selected slot with key presses
    void select_left();
    void select_right();
    void select_down();
    void select_up();

    void select_horizontal(int key);
    void select_vertical(int key);

    // returns true if coords (x,y) are within the bounds of the inventory
    bool is_within_bounds(float x, float y);

    int get_index(float x, float y);
    void select_block(float x, float y);

    virtual void toggle_mode(bool open);

    virtual void create() override;
    virtual void draw(ShaderProgram *slot_prog, Texture &slotTexture, ShaderProgram *block_prog);

    virtual ~Inventory() {}
};

#endif // INVENTORY_H
