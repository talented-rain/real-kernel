/*
 * Hardware Abstraction Layer Device-Tree
 *
 * File Name:   hal_of.c
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2023.06.03
 *
 * Copyright (c) 2023   Yang Yujun <yujiantianhu@163.com>
 *
 */

/*!< The includes */
#include <platform/of/hal_of.h>
#include <platform/of/hal_of_prop.h>

/*!< -------------------------------------------------------------------------- */
/*!< The defines */
/*!< Direct manipulation of the device tree address is not allowed, and the "of function" interface should be used */
#define RET_HAL_FDT_ENTRY								(ptr_fdt_memBuffer)
#define INIT_HAL_FDT_ENTRY(ptr)							(ptr_fdt_memBuffer = (ptr))

/*!< Address offset, and automatic 4-byte alignment */
#define FDT_PTR_MOVE_BYTE(ptr, len)						({(ptr) += (len); mrt_ptr_align4(ptr);})

/*!< -------------------------------------------------------------------------- */
/*!< The globals */
kuint8_t *ptr_fdt_memBuffer	= mrt_nullptr;
/*!< Global list */
struct hal_device_node *sprt_hal_of_allNodes = mrt_nullptr;

/*!< -------------------------------------------------------------------------- */
/*!< The functions */
static void hal_early_init_dt_params(void *ptr_dt);
static void hal_unflatten_device_tree(void);
static void __hal_unflatten_device_tree(struct hal_fdt_header *ptr_blob, struct hal_device_node ***sprt_allNext);
static void *hal_unflatten_dt_nodes(struct hal_fdt_header *ptr_blob,
									void *ptr_mem,
									void **ptr_start,
									struct hal_device_node ***sprt_allNext);
static void *hal_fdt_populate_node(struct hal_fdt_header *ptr_blob,
									void **ptr_offset, void **mem, void **ptr_parent, void ***allNext);
static void *hal_fdt_populate_properties(struct hal_fdt_header *ptr_blob,
									void **ptr_offset, void **mem, void *node, void ***allNext, kbool_t *has_name);
static void *hal_fdt_add_string_properties(void **mem, void *node, kstring_t *name, kstring_t *value, kuint32_t size, void ***allNext);
static void *hal_fdt_memory_calculate(void **mem, kuint32_t size, kuint32_t align);
static void *hal_fdt_memory_alloc(kuint32_t size, kuint32_t align);

/*!< -------------------------------------------------------------------------- */
/*!< API function */
/*!
 * @brief   setup_machine_fdt
 * @param   none
 * @retval  none
 * @note    none
 */
void setup_machine_fdt(void *ptr)
{
	kuint8_t *ptr_fdt_start;

	ptr_fdt_start = ptr;
	if (!mrt_isValid(ptr_fdt_start) || !hal_early_init_dt_verify(ptr_fdt_start))
	{
		return;
	}

	/*!< Initialize parameters */
	hal_early_init_dt_params(ptr_fdt_start);

	/*!< Parse the device tree */
	hal_unflatten_device_tree();
}

/*!
 * @brief   destroy_machine_fdt
 * @param   none
 * @retval  none
 * @note    none
 */
void destroy_machine_fdt(void)
{
	struct hal_device_node **sprt_mem = &sprt_hal_of_allNodes;

	if (mrt_isValid(*sprt_mem))
	{
		kfree(*sprt_mem);
		*sprt_mem = mrt_nullptr;
	}
}

/*!< ----------------------------------------------------------------------- */
/*!
 * @brief   Global resource initialization
 * @param   none
 * @retval  none
 * @note    none
 */
static void hal_early_init_dt_params(void *ptr_dt)
{
	if (!mrt_isValid(RET_HAL_FDT_ENTRY))
	{
		INIT_HAL_FDT_ENTRY(ptr_dt);
	}

	/*!< Initial global list */
	sprt_hal_of_allNodes = mrt_nullptr;
}

/*!
 * @brief   hal_unflatten_device_tree
 * @param   none
 * @retval  none
 * @note    none
 */
static void hal_unflatten_device_tree(void)
{
	struct hal_device_node **sprt_allNodes;
	void *ptr_fdt_start;

	ptr_fdt_start = (void *)RET_HAL_FDT_ENTRY;
	sprt_allNodes = &sprt_hal_of_allNodes;

	/*!< Parse the device tree */
	/*!<
	 * By the way:
	 * Although the address is stored on the Big and Littile endian of the CPU architecture,
	 * the value of each member is stored on the Big endian after being converted to the struct fdt_header
	 * Fortunately, each member of the struct fdt_header is of 4-byte type, and it is possible to convert the endian
	 */
	__hal_unflatten_device_tree(ptr_fdt_start, &sprt_allNodes);
}

/*!
 * @brief   __hal_unflatten_device_tree
 * @param   none
 * @retval  none
 * @note    none
 */
static void __hal_unflatten_device_tree(struct hal_fdt_header *ptr_blob, struct hal_device_node ***sprt_allNext)
{
	struct hal_device_node **sprt_allNodes;
	void *ptr_start;
	void *ptr_dt_mem;
	kusize_t size;

	if (!mrt_isValid(ptr_blob))
	{
		return;
	}

	/*!< Verify magic */
	if (FDT_MAGIC_VERIFY != FDT_TO_ARCH_ENDIAN32(ptr_blob->magic))
	{
		return;
	}

	sprt_allNodes = *sprt_allNext;

	/*!< Offset to the first address of the device block, this area will be used to build the device tree node */
	ptr_start = (void *)((void *)ptr_blob + FDT_TO_ARCH_ENDIAN32(ptr_blob->off_dt_struct));
	size = (kusize_t)hal_unflatten_dt_nodes(ptr_blob, mrt_nullptr, &ptr_start, mrt_nullptr);
	size = mrt_num_align4(size);
	if (!size)
	{
		return;
	}

	/*!< Apply for contiguous memory space */
	ptr_dt_mem = (void *)hal_fdt_memory_alloc(size + 4, __alignof__(struct hal_device_node));
	if (!mrt_isValid(ptr_dt_mem))
	{
		return;
	}

	/*!< The end is filled with a magic number, which is used to identify if the memory is out of bounds */
	*(kuint32_t *)(ptr_dt_mem + size) = FDT_TO_ARCH_ENDIAN32(FDT_MAGIC_VERIFY);

	/*!< The last call caused the ptr_start to shift and need to be back to the starting position */
	ptr_start = (void *)((void *)ptr_blob + FDT_TO_ARCH_ENDIAN32(ptr_blob->off_dt_struct));
	hal_unflatten_dt_nodes(ptr_blob, ptr_dt_mem, &ptr_start, &sprt_allNodes);

	/*!< Check the magic number, if the memory is out of bounds... it will not deal with it for the time being */
	if (FDT_MAGIC_VERIFY != FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)(ptr_dt_mem + size)))
	{
		return;
	}
}

/*!
 * @brief   Resolve all nodes
 * @param   none
 * @retval  none
 * @note    none
 */
static void *hal_unflatten_dt_nodes(struct hal_fdt_header *ptr_blob,
									void *ptr_mem,
									void **ptr_start,
									struct hal_device_node ***sprt_allNext)
{
	struct hal_device_node *sprt_node;
	struct hal_device_node **sprt_allNodes;
	struct hal_device_node *sprt_list;
	struct hal_device_node *sprt_cast;
	void *ptr_move;
	kuint32_t iTag;

	if (!mrt_isValid(ptr_blob) || !mrt_isValid(ptr_start))
	{
		return ptr_mem;
	}

	/*!< Obtain the first address of the device block */
	ptr_move = *ptr_start;
	/*!< iTag: Node labels */
	iTag = FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

	/*!< Not the start of the node, exiting incorrectly */
	if (FDT_ALL_NODE_START != iTag)
	{
		return ptr_mem;
	}

	sprt_node		= mrt_nullptr;
	sprt_list		= mrt_nullptr;
	sprt_allNodes	= mrt_isValid(sprt_allNext) ? *sprt_allNext : mrt_nullptr;

	/*!<
	 * Traverse all nodes in the DTB
	 * 1. Each node is populated by function hal_fdt_populate_node;
	 * 2. Child node
	 */
	while (Ert_true)
	{
		iTag = FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

		if (FDT_NODE_NOP == iTag)
		{
			ptr_move += 4;
			continue;
		}

		/*!<
		 * The node is directly connected to the FDT_NODE_START, indicating that there are child nodes;
		 * The node is directly connected to the FDT_NODE_END, which means that the node is finished and there are no child nodes
		 */
		if (FDT_NODE_END == iTag)
		{
			/*!< If the parent node is empty, it is the privilege of the root node, and the child node cannot be triggered */
			if (mrt_isValid(sprt_list))
			{
				sprt_cast = sprt_list;

				/*!< Go back to the previous node */
				sprt_list = sprt_list->parent;

				/*!< The child node has been used up, and the temporary memory is released */
				if (!mrt_isValid(sprt_allNodes))
				{
					kfree(sprt_cast);
				}
			}

			ptr_move += 4;
			continue;
		}

		/*!< All node traversal ends */
		if (FDT_ALL_NODE_END == iTag)
		{
			break;
		}

		/*!< Handle a single node */
		sprt_node = hal_fdt_populate_node(ptr_blob, &ptr_move, &ptr_mem, &sprt_list, &sprt_allNodes);
		if (!mrt_isValid(sprt_node))
		{
			/*!<
			 * If the memory request of the current node fails, you need to release the local node and the parent nodes at all levels;
			 * The sibling node has been released in the if (FDT_NODE_END == iTag), and there is no need to consider the existence of the sibling node
			 */
			if (!mrt_isValid(sprt_allNodes))
			{
				while (mrt_isValid(sprt_list))
				{
					sprt_cast = sprt_list;
					sprt_list = sprt_list->parent;
					kfree(sprt_cast);
				}

				return mrt_nullptr;
			}
		}

		sprt_list = sprt_node;
	}

	return ptr_mem;
}

/*!
 * @brief   Single-node handlers
 * @param   ptr_blob: 	dtb start address
 * @param	ptr_offset: the address that is currently offset
 * @param	mem: 		the first address where the node's memory is allocated
 * @param	parent: 	parent node
 * @param	allNext:	next node
 * @retval  allocated node memory pointer
 * @note    Parent, child, and sibling nodes are treated equally
 */
static void *hal_fdt_populate_node(struct hal_fdt_header *ptr_blob,
										void **ptr_offset, void **mem, void **ptr_parent, void ***allNext)
{
	struct hal_device_node *sprt_node;
	struct hal_device_node *sprt_parent;
	struct hal_of_property *sprt_prop;
	struct hal_of_property **sprt_prev;
	void *ptr_move, *ptr_mem, **ptr_allNext;
	kstring_t *ptr_path;
	kuint32_t iTag;
	kusize_t  ipathLenth, iLenthNeed;
	kstring_t *ptr_fullName;
	kbool_t   has_name, new_format;

	sprt_parent	= (struct hal_device_node *)(*ptr_parent);
	ptr_move	= *ptr_offset;
	ptr_mem		= *mem;
	ptr_allNext	= *allNext;
	new_format	= Ert_false;
	iTag		= FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

	/*!< Check: Whether the node is the start */
	if (FDT_NODE_START != iTag)
	{
		return mrt_nullptr;
	}

	/*!< Skip the iTag and point to the node name */
	/*!<
	 * ptr_path:	Node name, stored as a string
	 * ipathLenth:	Node name length, containing '\0'
	 */
	ptr_move   += 4;
	ptr_path	= (kstring_t *)ptr_move;
	ipathLenth	= strlen(ptr_path) + 1;
	iLenthNeed	= ipathLenth;

	/*!<
	 * There are two kinds of nodes:
	 *		one is the root node, which is characterized by ptr_path = '\0'; 
	 * 		The other is a child node, which is characterized by ptr_path = "node_name" '\0'
	 * For a path with nodes, the root node is represented as ptr_path = "/"'\0', and the first character of each node is '/'
	 */
	if ('/' != (*ptr_path))
	{
		/*!< sprt_node->fullname requires the name of the full path, so the length of the name of the full path must be calculated */
		new_format = Ert_true;

		/*!< If the parent node does not exist, this is the root node */
		if (!mrt_isValid(sprt_parent))
		{
			/*!< '/' + '\0', The total number of characters = 2 */
			iLenthNeed	= 2;
			ipathLenth	= 1;
			*ptr_path	= '\0';
		}
		else
		{
			/*!< The full-path name of the parent node + '/' (ipathLenth already contains the character '\0', and then add '/') */
			iLenthNeed	= ipathLenth + 1 + strlen(sprt_parent->full_name);
		}
	}

	sprt_node = hal_fdt_memory_calculate(&ptr_mem, sizeof(struct hal_device_node) + iLenthNeed, __alignof__(struct hal_device_node));

	if (mrt_isValid(ptr_allNext))
	{
		/*!< Inserts the current node into the list */
		*ptr_allNext = sprt_node;
		ptr_allNext	 = &sprt_node->allnext;

		/*!< Update the address */
		*allNext = ptr_allNext;
	}
	else
	{
		/*!< 
		 * Request temporary memory; 
		 * This method makes the memory of each node discontinuous, so it is only used temporarily, and must be released when it is used up 
		 */
		sprt_node = kzalloc(sizeof(struct hal_device_node) + iLenthNeed, GFP_KERNEL);
		if (!mrt_isValid(sprt_node))
		{
			return mrt_nullptr;
		}
	}

	/*!< Point to (mem - iLenthNeed), which is dedicated to storing pathnames */
	sprt_node->full_name = (kstring_t *)(sprt_node + 1);
	ptr_fullName = sprt_node->full_name;

	if (new_format)
	{
		if (mrt_isValid(sprt_parent) && mrt_isValid(sprt_parent->parent))
		{
			/*!< Copies the pathname of the parent node */
			strcpy(ptr_fullName, sprt_parent->full_name);
			ptr_fullName += strlen(sprt_parent->full_name);
		}

		*(ptr_fullName++) = '/';
	}

	/*!< Final Form: "Parent Node Pathname/Local Node Name" */
	memcpy(ptr_fullName, ptr_path, ipathLenth);
	/*!< The child node points to the parent node */
	sprt_node->parent = sprt_parent;

	/*!< Initialize the value of phandle */
	sprt_node->phandle = -1;

	if (mrt_isValid(sprt_parent) && mrt_isValid(ptr_allNext))
	{
		/*!< The parent node points to the child node, completing the list */
		/*!< This node is the first child node */
		if (!mrt_isValid(sprt_parent->sprt_next))
		{
			/*!< child saves the first child node */
			sprt_parent->child = sprt_node;
		}

		/*!< The parent node already has a child node, that is, this node is a sibling node of the previous child node */
		else
		{
			sprt_parent->sprt_next->sibling = sprt_node;
		}

		sprt_parent->sprt_next = sprt_node;
	}

	/*!<
	 * When does it need byte alignment?
	 * The length of the node name is generally not fixed, 
	 * and it is not possible to accurately make it exactly 4 bytes every time, and those less than 4 bytes often need to be filled with 0
	 * The operation of complementing 0 is byte alignment
	 * For instance:
	 * 		ptr_path = "12345", ipathLenth = 5 + 1 = 6
	 * 		Hexadecimal is, ptr_path = 01 02 03 04 05 00 00 00, the first 00 is '\0', The last two 00 are byte-aligned (complement 0)
	 * (The string is stored in a single byte, so big-endian and little-endian are all on its own)
	 */
	has_name	= Ert_false;
	sprt_prev	= &sprt_node->properties;
	ptr_move	= FDT_PTR_MOVE_BYTE(ptr_move, ipathLenth);

	/*!< Traverse the properties of this node */
	while (Ert_true)
	{
		iTag = FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

		if (FDT_NODE_NOP == iTag)
		{
			ptr_move += 4;
			continue;
		}

		/*!< Handles individual properties under this node and automatically completes the pointer position offset */
		sprt_prop = hal_fdt_populate_properties(ptr_blob, &ptr_move, &ptr_mem, sprt_node, &ptr_allNext, &has_name);
		if (!mrt_isValid(sprt_prop))
		{
			break;
		}

		/*!< Inserts the attribute into the linked list of the local node's attributes */
		if (mrt_isValid(ptr_allNext))
		{
			*sprt_prev	= sprt_prop;
			sprt_prev	= &sprt_prop->sprt_next;
		}
	}

	/*!< The property traversal is complete, and some work should be done before closing: */
	/*!< This node has been traversed, but the name property is not found: create one manually */
	if (!has_name)
	{
		kstring_t *ptr_1, *ptr_2, *ptr_3;

		ptr_1 = sprt_node->full_name;
		ptr_2 = sprt_node->full_name;
		ptr_3 = mrt_nullptr;

		/*!<
		 * Create Rule: Captures the name between the path and the unit
		 * Such as node fullname = "/cpus/core@0", name property = "core"
		 */
		while (*ptr_1)
		{
			/*!<
			 * ptr_3: Record the position of '@', and you can also use this position as '\0' when calculating the length
			 * ptr_2: The position of the first character after updating '/'
			 * the lenth of name property = (ptr_3 + 1) - ptr_2
			 */
			ptr_3 = ('@' == (*ptr_1)) ? (ptr_1) 	: ptr_3;
			ptr_2 = ('/' == (*ptr_1)) ? (ptr_1 + 1) : ptr_2;

			ptr_1++;
		}

		/*!<
		 * If there is a way to write it: /cpus/my@core/cpu0, the last '/' is after '@';
		 * At this time ptr_1 has reached the end of the string, i.e.: *ptr_1 = '\0'
		 * Take ptr_3 = ptr_1, then name's property lenth = (ptr_3 + 1) - ptr_2
		 */
		ptr_3 = (ptr_3 < ptr_2) ? ptr_1 : ptr_3;

		sprt_prop = hal_fdt_add_string_properties(&ptr_mem, sprt_node, "name", ptr_2, ((ptr_3 + 1) - ptr_2), &ptr_allNext);
		if (mrt_isValid(sprt_prop) && mrt_isValid(ptr_allNext))
		{
			/*!< Inserts the property into the list of the local node's properties */
			*sprt_prev	= sprt_prop;
			sprt_prev	= &sprt_prop->sprt_next;
		}
	}

	if (mrt_isValid(ptr_allNext))
	{
		if (!mrt_isValid(sprt_node->name))
		{
			sprt_node->name	= "<null>";
		}

		if (!mrt_isValid(sprt_node->type))
		{
			sprt_node->type	= "<null>";
		}
	}

	*mem		= ptr_mem;
	*ptr_offset	= ptr_move;

	return sprt_node;
}

/*!
 * @brief   Deal with one property
 * @param   none
 * @retval  none
 * @note    none
 */
static void *hal_fdt_populate_properties(struct hal_fdt_header *ptr_blob,
										void **ptr_offset, void **mem, void *node, void ***allNext, kbool_t *has_name)
{
	void *ptr_str_start;
	struct hal_device_node *sprt_node;
	struct hal_of_property *sprt_prop;
	void *ptr_move, *ptr_mem, **ptr_allNext, *ptr_value;
	kuint32_t tag;
	kstring_t *ptr_name;
	kuint32_t iPropOffset;
	kusize_t  iPropLenth;

	/*!< Gets the current pointer position */
	ptr_move	= *ptr_offset;
	ptr_mem		= *mem;
	sprt_node	= (struct hal_device_node *)node;
	ptr_allNext	= *allNext;
	tag			= FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

	/*!< Non-property values are not handled by this function */
	if (FDT_NODE_PROP != tag)
	{
		sprt_prop = mrt_nullptr;
		goto exit;
	}

	/*!< Skip tag */
	ptr_move += 4;

	/*!< FDT_NODE_PROP is followed by the lenth of the property value and the offset of the property value name in the string block; 4 bytes each */
	/*!< Gets the property value length */
	iPropLenth = FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);
	ptr_move += 4;

	/*!< Gets the offset of the property value name in the string block */
	/*!< The so-called property value name, i.e. something like: name, compatible, address_size, ... */
	iPropOffset	= FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);
	ptr_move += 4;

	/*!< If the device tree version is less than 10, you need to choose a different alignment method based on iPropLenth */
	if (FDT_TO_ARCH_ENDIAN32(ptr_blob->version) < 0x10)
	{
		ptr_move = mrt_ptr_align(ptr_move, (iPropLenth >= 8) ? 8 : 4);
	}

	/*!< Get property value */
	/*!<If the name is compatible, ptr_prop that is the content of compatible */
	ptr_value = (void *)ptr_move;

	/*!<
	 * Navigate to the first address of the string block
	 * Get property name
	 */
	ptr_str_start	= (void *)((void *)ptr_blob + FDT_TO_ARCH_ENDIAN32(ptr_blob->off_dt_strings));
	ptr_name		= (kstring_t *)(ptr_str_start + iPropOffset);
	*has_name		= (!strcmp("name", ptr_name)) ? Ert_true : (*has_name);

	/*!< Record this property */
	sprt_prop = hal_fdt_memory_calculate(&ptr_mem, sizeof(struct hal_of_property), __alignof__(struct hal_of_property));

	/*!<
	 * allnext:
	 */
	if (mrt_isValid(ptr_allNext))
	{
		sprt_prop->name	= ptr_name;
		sprt_prop->length = iPropLenth;
		sprt_prop->value = ptr_value;
		sprt_prop->sprt_next = mrt_nullptr;

		/*!< Fill node information */
		if (!strcmp("name", sprt_prop->name))
		{
			sprt_node->name	= (kstring_t *)sprt_prop->value;
		}
		else if (!strcmp("device_type", sprt_prop->name))
		{
			sprt_node->type	= (kstring_t *)sprt_prop->value;
		}
		else if (!strcmp("phandle", sprt_prop->name))
		{
			sprt_node->phandle = FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)sprt_prop->value);
		}
	}

	/*!< If the next one is not an empty node, nor is it a property, then the property traversal ends */
	ptr_move = FDT_PTR_MOVE_BYTE(ptr_move, iPropLenth);
	tag	= FDT_TO_ARCH_ENDIAN32(*(kuint32_t *)ptr_move);

exit:
	/*!< Update the pointer position */
	*mem		= ptr_mem;
	*ptr_offset	= ptr_move;

	return sprt_prop;
}

/*!
 * @brief   Add string property
 * @param   none
 * @retval  none
 * @note    If you want to add an integer property, build a separate function and then switch to big-endian mode when storing value
 */
static void *hal_fdt_add_string_properties(void **mem, void *node, kstring_t *name, kstring_t *value, kuint32_t size, void ***allNext)
{
	struct hal_device_node *sprt_node;
	struct hal_of_property *sprt_prop;
	void *ptr_mem, **ptr_allNext;

	ptr_mem		= *mem;
	sprt_node	= (struct hal_device_node *)node;
	ptr_allNext	= *allNext;

	if (!mrt_isValid(name))
	{
		sprt_prop = mrt_nullptr;
		goto exit;
	}

	sprt_prop = hal_fdt_memory_calculate(&ptr_mem, sizeof(struct hal_of_property) + size, __alignof__(struct hal_of_property));

	/*!< Inserts the property into the linked list of the local node's properties */
	if (mrt_isValid(ptr_allNext))
	{
		sprt_prop->name	= name;
		sprt_prop->length = size;
		sprt_prop->sprt_next = mrt_nullptr;

		sprt_prop->value = (void *)(sprt_prop + 1);
		memcpy(sprt_prop->value, value, size - 1);
		*(kstring_t *)(sprt_prop->value + size - 1)	= '\0';

		/*!< Fill node information */
		if (!strcmp("name", sprt_prop->name))
		{
			sprt_node->name	= (kstring_t *)sprt_prop->value;
		}
		else if (!strcmp("device_type", sprt_prop->name))
		{
			sprt_node->type	= (kstring_t *)sprt_prop->value;
		}
	}

exit:
	/*!< Update the pointer position */
	*mem = ptr_mem;

	return sprt_prop;
}

/*!
 * @brief   Memory space calculation
 * @param   none
 * @retval  none
 * @note    Start with (void *)0 and add up to get the final mem
 */
static void *hal_fdt_memory_calculate(void **mem, kuint32_t size, kuint32_t align)
{
	void *res;

	*mem = mrt_ptr_align(*mem, align);
	res = *mem;
	*mem += size;

	return res;
}

/*!
 * @brief   Memory space allocation
 * @param   none
 * @retval  none
 * @note    none
 */
static void *hal_fdt_memory_alloc(kuint32_t size, kuint32_t align)
{
	void *mem = mrt_nullptr;

	mem = mrt_ptr_align(mem + size, align);
	size = (kuaddr_t)mem;

	return kzalloc(size, GFP_KERNEL);
}

/*!< ---------------------------------------------------------------------------------- */
/*!
 * @brief   Find nodes based on paths
 * @param   none
 * @retval  none
 * @note    The essence is to compare the fullname of each node
 */
struct hal_device_node *hal_of_find_node_by_path(const kstring_t *ptr_path)
{
	struct hal_device_node *sprt_list = mrt_nullptr;
	struct hal_device_node *sprt_head = mrt_hal_fdt_node_header();

	foreach_list_odd(sprt_head, sprt_list, allnext)
	{
		if (!strcmp(ptr_path, sprt_list->full_name))
		{
			break;
		}
	}

	return sprt_list;
}

/*!
 * @brief   Find nodes by name
 * @param   none
 * @retval  none
 * @note    The essence is to compare the name property of each node
 */
struct hal_device_node *hal_of_find_node_by_name(struct hal_device_node *sprt_from, const kstring_t *ptr_name)
{
	struct hal_device_node *sprt_list = mrt_nullptr;
	struct hal_device_node *sprt_head = mrt_isValid(sprt_from) ? sprt_from : mrt_hal_fdt_node_header();

	foreach_list_odd(sprt_head, sprt_list, allnext)
	{
		if (!strcmp(ptr_name, sprt_list->name))
		{
			break;
		}
	}

	return sprt_list;
}

/*!
 * @brief   Finds nodes based on type
 * @param   none
 * @retval  none
 * @note    The essence is to compare the type property of each node
 */
struct hal_device_node *hal_of_find_node_by_type(struct hal_device_node *sprt_from, const kstring_t *ptr_type)
{
	struct hal_device_node *sprt_list = mrt_nullptr;
	struct hal_device_node *sprt_head = mrt_isValid(sprt_from) ? sprt_from : mrt_hal_fdt_node_header();

	foreach_list_odd(sprt_head, sprt_list, allnext)
	{
		if (!strcmp(ptr_type, sprt_list->type))
		{
			break;
		}
	}

	return sprt_list;
}

/*!
 * @brief   Find nodes by phandle
 * @param   none
 * @retval  none
 * @note    The essence is to compare the value of phandle of each node
 */
struct hal_device_node *hal_of_find_node_by_phandle(struct hal_device_node *sprt_from, kuint32_t phandle)
{
	struct hal_device_node *sprt_list = mrt_nullptr;
	struct hal_device_node *sprt_head = mrt_isValid(sprt_from) ? sprt_from : mrt_hal_fdt_node_header();

	foreach_list_odd(sprt_head, sprt_list, allnext)
	{
		if (phandle == sprt_list->phandle)
		{
			break;
		}
	}

	return sprt_list;
}

/*!
 * @brief   Find nodes based on properties
 * @param   none
 * @retval  none
 * @note    The essence is to compare the type property and the compatible property of each node; type can be NULL
 */
struct hal_device_node *hal_of_find_compatible_node(struct hal_device_node *sprt_from,
										const kstring_t *ptr_type, const kstring_t *ptr_compat)
{
	struct hal_device_node *sprt_list = mrt_nullptr;
	struct hal_device_node *sprt_head = mrt_isValid(sprt_from) ? sprt_from : mrt_hal_fdt_node_header();

	foreach_list_odd(sprt_head, sprt_list, allnext)
	{
		if (mrt_isValid(ptr_type) && strcmp(ptr_type, sprt_list->type))
		{
			continue;
		}

		if (hal_of_device_is_compatible(sprt_list, ptr_compat))
		{
			break;
		}
	}

	return sprt_list;
}

/*!
 * @brief   Complete the matching based on the idtable
 * @param   none
 * @retval  none
 * @note    try to match node and matches
 */
struct hal_device_node *hal_of_node_try_matches(struct hal_device_node *sprt_node,
							const struct hal_of_device_id *sprt_matches, struct hal_of_device_id **sprt_match)
{
	struct hal_of_device_id *sprt_match_id = (struct hal_of_device_id *)sprt_matches;

	if (!mrt_isValid(sprt_node))
	{
		goto fail;
	}

	while (mrt_isValid(sprt_match_id) && mrt_isValid(sprt_match_id->compatible))
	{
		if (hal_of_device_is_compatible(sprt_node, sprt_match_id->compatible))
		{
			if (mrt_isValid(sprt_match))
			{
				*sprt_match	= sprt_match_id;
			}

			return sprt_node;
		}

		sprt_match_id++;
	}

fail:
	if (mrt_isValid(sprt_match))
	{
		*sprt_match	= mrt_nullptr;
	}

	/*!< The purpose of the function is unknown, and it is not implemented at the moment */
	return mrt_nullptr;
}

/*!
 * @brief   Complete the matching based on the idtable
 * @param   none
 * @retval  none
 * @note    The purpose of the function is unknown, and it is not implemented at the moment
 */
struct hal_device_node *hal_of_find_matching_node_and_match(struct hal_device_node *sprt_from,
							const struct hal_of_device_id *sprt_matches, struct hal_of_device_id **sprt_match)
{
	struct hal_device_node *sprt_node;

	FOREACH_OF_DT_NODE(sprt_node, sprt_from)
	{
		if (sprt_node == hal_of_node_try_matches(sprt_node, sprt_matches, sprt_match))
		{
			return sprt_node;
		}
	}

	if (mrt_isValid(sprt_match))
	{
		*sprt_match	= mrt_nullptr;
	}

	/*!< The purpose of the function is unknown, and it is not implemented at the moment */
	return mrt_nullptr;
}

/*!
 * @brief   Gets the parent node of the current node
 * @param   none
 * @retval  none
 * @note    none
 */
struct hal_device_node *hal_of_get_parent(struct hal_device_node *sprt_node)
{
	return (mrt_isValid(sprt_node) ? sprt_node->parent : mrt_nullptr);
}

/*!
 * @brief   Get each child of the parent node
 * @param   none
 * @retval  none
 * @note    With the outer loop, all child nodes can be obtained
 */
struct hal_device_node *hal_of_get_next_child(struct hal_device_node *sprt_node, struct hal_device_node *ptr_prev)
{
	/*!< child holds the first child node */

	/*!< The purpose of the function is unknown, and it is not implemented at the moment */
	return mrt_nullptr;
}

/*!
 * @brief   none
 * @param   none
 * @retval  none
 * @note    retval: [true: ok, okay; false: disable]
 */
kbool_t hal_of_device_is_avaliable(struct hal_device_node *sprt_node)
{
	kstring_t *ptr_value;

	ptr_value = (kstring_t *)hal_of_get_property(sprt_node, "status", mrt_nullptr);

	if (!mrt_isValid(ptr_value) || !strcmp(ptr_value, "ok") || !strcmp(ptr_value, "okay"))
	{
		return Ert_true;
	}

	else if (!strcmp(ptr_value, "disabled"))
	{
		return Ert_false;
	}

	return Ert_false;
}

/*!
 * @brief   Compare matches
 * @param   none
 * @retval  none
 * @note    If the value of the compatible property is the same as one of the matches, return true
 */
struct hal_of_device_id *hal_of_match_node(const struct hal_of_device_id *sprt_matches, struct hal_device_node *sprt_node)
{
	struct hal_of_device_id *sprt_match = (struct hal_of_device_id *)sprt_matches;

	while (mrt_isValid(sprt_match) && mrt_isValid(sprt_match->compatible))
	{
		if (hal_of_device_is_compatible(sprt_node, (sprt_match++)->compatible))
		{
			return (--sprt_match);
		}
	}

	return mrt_nullptr;
}

/*!
 * @brief   Get the cells of irq priority
 * @param   none
 * @retval  none
 * @note    none
 */
struct hal_device_node *hal_of_irq_parent(struct hal_device_node *sprt_node)
{
	struct hal_device_node *sprt_root, *sprt_np = sprt_node;
	void *p = mrt_nullptr;
	kuint32_t handle;
	ksint32_t retval;

	sprt_root = hal_of_find_node_by_path("/");

	while (mrt_isValid(sprt_np) && (!mrt_isValid(p)))
	{
		retval = hal_of_property_read_u32(sprt_np, "interrupt-parent", &handle);
		if (!mrt_isErr(retval))
		{
			sprt_np = hal_of_find_node_by_phandle(sprt_root, handle);
			if (mrt_isValid(sprt_np))
			{
				goto LOOP;
			}
		}

		retval = hal_of_property_read_u32(sprt_np, "interrupt-extended", &handle);
		if (!mrt_isErr(retval))
		{
			sprt_np = hal_of_find_node_by_phandle(sprt_root, handle);
			if (mrt_isValid(sprt_np))
			{
				goto LOOP;
			}
		}

		sprt_np = sprt_np->parent;

LOOP:
		p = mrt_isValid(sprt_np) ? hal_of_find_property(sprt_np, "#interrupt-cells", mrt_nullptr) : mrt_nullptr;
	}

	return sprt_np;
}

/*!
 * @brief   Get the cells of irq priority
 * @param   none
 * @retval  none
 * @note    none
 */
kuint32_t hal_of_n_irq_cells(struct hal_device_node *sprt_node)
{
	struct hal_device_node *sprt_np;
	kuint32_t cells = 0;

	sprt_np = hal_of_irq_parent(sprt_node);
	if (mrt_isValid(sprt_np))
	{
		/*!< Search "#interrupt-cells", from parent to parent */
		hal_of_property_read_u32(sprt_np, "#interrupt-cells", &cells);
	}

	return cells;
}

/*!
 * @brief   Get the numbers of irq
 * @param   none
 * @retval  none
 * @note    none
 */
kuint32_t hal_of_irq_count(struct hal_device_node *sprt_node)
{
	kuint32_t value, count, cells;
	ksint32_t retval;

	cells = hal_of_n_irq_cells(sprt_node);
	if (!mrt_isValid(cells))
	{
		return 0;
	}

	for (count = 0; ; count++)
	{
		retval = hal_of_property_read_u32_index(sprt_node, "interrupts", count * cells, &value);
		if (mrt_isErr(retval))
		{
			break;
		}
	}

	return count;
}
