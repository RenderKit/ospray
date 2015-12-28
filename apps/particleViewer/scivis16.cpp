/*
Data Format
-----------
- position - float x, float y, float z
- velocity - float x, float y, float z
- concentration - float x

The `magic_offset` points to the start of the binary raw data.


Notes
-----
- always 4 byte padding before each array
*/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "Model.h"

#include "scivis16.hpp"


namespace ospray {
namespace particle {

Model *parse_scivis16(char const *filepath)
{
        /* TODO: parse XML */
        enum { magic_offset = 4072, };

        FILE *fp = fopen(filepath, "rb");
        assert(fp);

        fseek(fp, magic_offset, SEEK_SET);


        /* process header */
        struct {
                uint32_t pad_size;
                uint32_t size;
                uint32_t pad_step;
                uint32_t step;
                uint32_t pad_time;
                float time;
        } header;

        fread(&header, sizeof header, 1, fp);


        Model *model = new Model;


        /* read particles position */
        fseek(fp, 4, SEEK_CUR);

        Model::AtomType *atom_type = new Model::AtomType("position");
        atom_type->color = vec3f(1.0f, 0.0f, 0.0f);
        model->atomType.push_back(atom_type);

        for (uint32_t i = 0; i < header.size; ++i) {
                float co[3];
                fread(co, sizeof co, 1, fp);

                Model::Atom atom;
                atom.position = vec3f(co[0], co[1], co[2]);
                atom.radius = 0.05f;
                atom.type = 0;
                model->atom.push_back(atom);
        }


        /* read particles velocity */
        fseek(fp, 4, SEEK_CUR);

        for (uint32_t i = 0; i < header.size; ++i) {
                float co[3];
                fread(co, sizeof co, 1, fp);

                model->addAttribute("velocity_x", co[0]);
                model->addAttribute("velocity_y", co[1]);
                model->addAttribute("velocity_z", co[2]);
        }


        /* read particles concentration */
        fseek(fp, 4, SEEK_CUR);

        for (uint32_t i = 0; i < header.size; ++i) {
                float concentration;
                assert(fread(concentration, sizeof concentration, 1, fp));

                model->addAttribute("concentration", concentration);
        }


        fclose(fp);

        return model;
}

}
}
