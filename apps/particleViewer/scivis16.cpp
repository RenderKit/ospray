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
                uint32_t _pad3;
                uint32_t size;
                uint32_t _pad1;
                uint32_t step;
                uint32_t _pad2;
                float time;
        } header;

        fread(&header, sizeof (header), 1, fp);


        Model *model = new Model;

        /* read particles */
        Model::AtomType *atom_type = new Model::AtomType("position");
        atom_type->color = vec3f(1.0f, 0.0f, 0.0f);
        model->atomType.push_back(atom_type);

        for (uint32_t i = 0; i < header.size; ++i) {
                float co[3];
                fread(co, sizeof (co), 1, fp);

                Model::Atom atom;
                atom.position = vec3f(co[0], co[1], co[2]);
                atom.radius = 0.05f;
                atom.type = 0;
                model->atom.push_back(atom);
        }


        fclose(fp);

        return model;
}

}
}
