#pragma once

#ifndef __WDFuzzing__
#define __WDFuzzing__

#include "wdissector.h"

void packet_navigate_cpp(wd_t *wd, uint32_t skip_layers, uint32_t skip_groups, std::function<uint8_t(proto_tree *, uint8_t, uint8_t *)> callback)
{
    epan_dissect_t *edt = wd_edt(wd);
    proto_tree *node = edt->tree;
    proto_tree *subnode = NULL, *subnode_parent;
    uint32_t layers_count = 0;
    uint32_t groups_count = 0;
    uint32_t level = 0;
    uint8_t ret = 1;

    if (!node || !callback)
        return;

    // 1. Root Tree Children (Layers) (Loop)
    node = node->first_child;
    while (node != NULL) {
        layers_count++;

        if (layers_count > skip_layers) {
            // 2. Node Children(Fields or other layers)(Loop)
            callback(node, WD_TYPE_LAYER, (uint8_t *)edt->tvb->real_data);
            // printf("==== Layer: %s, Type=%s, Size=%d\n", node->finfo->hfinfo->name, node->finfo->value.ftype->name, node->finfo->length);

            if ((subnode = node->first_child)) {
                level++;

                while (subnode != NULL) {
                    // 3.1 Field Group (Intermediary Node)
                    if (subnode->first_child != NULL) {
                        groups_count++;
                        // Only process groups not skipped
                        if ((groups_count > skip_groups) && (1 || subnode->finfo->length || subnode->first_child->finfo->length)) {

                            if (subnode->finfo->length) // Only process groups with fields not skipped
                            {
                                if (subnode->finfo->value.ftype->ftype == FT_PROTOCOL || subnode->parent->parent == NULL)
                                    ret = callback(subnode, WD_TYPE_LAYER, (uint8_t *)edt->tvb->real_data);
                                // printf("==== Layer: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                                else if (subnode->first_child->finfo->length)
                                    ret = callback(subnode, WD_TYPE_GROUP, (uint8_t *)edt->tvb->real_data);
                                // printf("---> Field Group: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                                else
                                    // Group with a children without size is actually a field
                                    ret = callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt->tvb->real_data);
                                // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));
                            }
                            // subnode_parent = subnode;
                            level++;
                            subnode = subnode->first_child;
                        }
                        else {
                            // Skip current group
                            if (subnode->next)
                                subnode = subnode->next;
                            else {
                                // Return to previous parent recursivelly
                                // puts("<--- Return");
                                level--;
                                subnode = subnode->parent;
                                while (subnode->next == NULL) {
                                    if (!subnode->parent) {
                                        // detect final subnode
                                        subnode = NULL;
                                        node = NULL;
                                        break;
                                    }

                                    // puts("<--- Return");
                                    level--;
                                    subnode = subnode->parent;
                                }
                                if (subnode)
                                    subnode = subnode->next;
                            }
                        }
                    }
                    // 3.2 Leaf (Child Node)
                    else if (subnode->next != NULL) {
                        if (subnode->finfo->length && subnode->finfo->value.ftype->ftype != FT_PROTOCOL && subnode->finfo->value.ftype->ftype != FT_NONE) // Only process fields not skipped
                            ret = callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt->tvb->real_data);
                        // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));

                        subnode = subnode->next;
                    }
                    // 3.3 Final Leaf (Last Child Node)
                    else {
                        if (subnode->finfo->length && subnode->finfo->value.ftype->ftype != FT_NONE) // Only process fields not skipped
                            ret = callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt->tvb->real_data);
                        // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));

                        // Return to previous parent recursivelly
                        // puts("<--- Return");
                        level--;
                        subnode = subnode->parent;
                        while (subnode->next == NULL) {
                            if (!subnode->parent) {
                                // detect final subnode
                                subnode = NULL;
                                node = NULL;
                                break;
                            }

                            // puts("<--- Return");
                            level--;
                            subnode = subnode->parent;
                        }
                        if (subnode)
                            subnode = subnode->next;
                    }
                    if (!ret) {
                        // Exit packet navigation if callback returns 0
                        subnode = NULL;
                        node = NULL;
                        break;
                    }
                }
            }
        }

        if (node)
            node = node->next;
    }
}

#endif
