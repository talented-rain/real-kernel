/*
 * Hardware Abstraction Layer Pinctrl Interface
 *
 * File Name:   fwk_pinctrl.h
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2024.04.15
 *
 * Copyright (c) 2024   Yang Yujun <yujiantianhu@163.com>
 *
 */

#ifndef __FWK_PINCTRL_H_
#define __FWK_PINCTRL_H_

/*!< The includes */
#include <platform/fwk_basic.h>
#include <platform/fwk_platform.h>
#include <platform/of/fwk_of.h>
#include <kernel/kernel.h>
#include <kernel/mutex.h>

/*!< The defines */
typedef struct fwk_gpio_chip srt_fwk_gpio_chip_t;

#define FWK_PINCTRL_STATE_DEFAULT 					"default"
#define FWK_PINCTRL_STATE_IDLE 						"idle"
#define FWK_PINCTRL_STATE_SLEEP 					"sleep"

#define FWK_PINCTRL_STATE_DUMMY						"<dummy>"
#define FWK_PINCTRL_PIN_DUMMY						"<dummy>"

enum __ERT_FWK_PINCTRL_PIN_TYPE
{
	NR_FWK_PINCTRL_PIN_MUX = 0,
	NR_FWK_PINCTRL_PIN_CONF,
};

typedef struct fwk_pinctrl_pin_desc 
{
	kuint32_t number;								/*!< pin number */
	const kstring_t *name;							/*!< pin name */
	void *drv_data;

#define FWK_PINCTRL_PIN(a, b)						{ .number = a, .name = b }
	
} srt_fwk_pinctrl_pin_desc_t;

typedef struct fwk_pinctrl_desc
{
	const kstring_t *name;

	/*!< npins: sizeof(sprt_pins[])*/
	struct fwk_pinctrl_pin_desc const *sprt_pins;
	kuint32_t npins;

	const struct fwk_pinctrl_ops *sprt_pctlops;
	const struct fwk_pinmux_ops *sprt_pmxops;
	const struct fwk_pinconf_ops *sprt_confops;

} srt_fwk_pinctrl_desc_t;

typedef struct fwk_pinctrl_dev 
{
	struct list_head sgrt_link;						/*!< link to global list */
	struct fwk_pinctrl_desc *sprt_desc;
	struct list_head sgrt_gpio_ranges;
	struct fwk_device *sprt_dev;
	struct fwk_pinctrl_state *sprt_hog_default;
	struct fwk_pinctrl_state *sprt_hog_sleep;
	struct mutex_lock sgrt_mutex;
	struct fwk_pinctrl *sprt_pctl;

	void *driver_data;

} srt_fwk_pinctrl_dev_t;

/*!< ----------------------------------------------------------------- */
/*!< pinctrl ctrl operations */
typedef struct fwk_pinctrl_map_mux 
{
	const kstring_t *group;
	const kstring_t *function;

} srt_fwk_pinctrl_map_mux_t;

typedef struct fwk_pinctrl_map_configs 
{
	const kstring_t *group_or_pin;
	kuint32_t *configs;
	kuint32_t num_configs;

} srt_fwk_pinctrl_map_configs_t;

typedef struct fwk_pinctrl_map 
{
	const kstring_t *dev_name;
	const kstring_t *name;
	kuint32_t type;
	const kstring_t *ctrl_dev_name;

	union 
	{
		struct fwk_pinctrl_map_mux sgrt_mux;
		struct fwk_pinctrl_map_configs sgrt_configs;
	} ugrt_data;

} srt_fwk_pinctrl_map_t;

typedef struct fwk_pinctrl_maps 
{
	struct list_head sgrt_link;
	struct fwk_pinctrl_map *sprt_maps;
	kuint32_t num_maps;

} srt_fwk_pinctrl_maps_t;

typedef struct fwk_pinctrl_ops
{
	ksint32_t 		(*get_function_groups) (srt_fwk_pinctrl_dev_t *sprt_pctldev,
				  						kuint32_t selector, kuaddr_t **groups, kuint32_t * const num_groups);
	ksint32_t 		(*get_groups_count)(srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t func_selector);
	const kstring_t *(*get_group_name) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t func_selector, kuint32_t group_selector);
	const kstring_t *(*get_pin_desc)(srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t number);

	ksint32_t 		(*dt_node_to_map) (srt_fwk_pinctrl_dev_t *sprt_pctldev, srt_fwk_device_node_t *sprt_node, 
												srt_fwk_pinctrl_map_t **sprt_map, kuint32_t *num_maps);
	void 			(*dt_free_map) (srt_fwk_pinctrl_dev_t *sprt_pctldev, srt_fwk_pinctrl_map_t *sprt_map, kuint32_t num_maps);

} srt_fwk_pinctrl_ops_t;

/*!< ----------------------------------------------------------------- */
/*!< pinctrl pinmux operations */
typedef struct fwk_pinctrl_gpio_range 
{
	struct list_head sgrt_link;
	const kstring_t *name;
	kuint32_t id;
	kuint32_t base;
	kuint32_t pin_base;
	kuint32_t const *pins;
	kuint32_t npins;

	struct fwk_gpio_chip *sprt_gc;

} srt_fwk_pinctrl_gpio_range_t;

typedef struct fwk_pinmux_ops 
{
	ksint32_t 		(*request) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t offset);
	ksint32_t 		(*free) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t offset);

	ksint32_t 		(*get_functions_count) (srt_fwk_pinctrl_dev_t *sprt_pctldev);
	const kstring_t *(*get_function_name) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t selector);
	ksint32_t 		(*set_mux) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t func_selector, kuint32_t group_selector);
	
	ksint32_t 		(*gpio_request_enable) (srt_fwk_pinctrl_dev_t *sprt_pctldev, 
										srt_fwk_pinctrl_gpio_range_t *sprt_range, kuint32_t offset);

	void 			(*gpio_disable_free) (srt_fwk_pinctrl_dev_t *sprt_pctldev, 
										srt_fwk_pinctrl_gpio_range_t *sprt_range, kuint32_t offset);

	ksint32_t 		(*gpio_set_direction) (srt_fwk_pinctrl_dev_t *sprt_pctldev, 
										srt_fwk_pinctrl_gpio_range_t *sprt_range, kuint32_t offset, kbool_t input);
	
} srt_fwk_pinmux_ops_t;

/*!< ----------------------------------------------------------------- */
/*!< pinctrl pin configurations operations */
typedef struct fwk_pinconf_ops 
{
	ksint32_t 		(*pin_config_get) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t pin, kuint32_t *config);
	ksint32_t 		(*pin_config_set) (srt_fwk_pinctrl_dev_t *sprt_pctldev, kuint32_t pin, kuint32_t *configs, kuint32_t num_configs);

} srt_fwk_pinconf_ops_t;

/*!< ----------------------------------------------------------------- */
typedef struct fwk_pinctrl_mux 
{
	kuint32_t group;
	kuint32_t func;										/*!< mux data*/

} srt_fwk_pinctrl_mux_t;

typedef struct fwk_pinctrl_configs 
{
	kuint32_t group_or_pin;								/*!< group, such as i2c: sck and sda are one group */
	kuint32_t *configs;
	kuint32_t num_configs;

} srt_fwk_pinctrl_configs_t;

/*!< pin configurations of one pin */
typedef struct fwk_pinctrl_setting 
{
	kuint32_t type;										/*!< refer to "__ERT_FWK_PINCTRL_PIN_TYPE" */
	struct list_head sgrt_link;							/*!< link to fwk_pinctrl_state::sgrt_settings */
	struct fwk_pinctrl_dev *sprt_pctldev;
	const kstring_t *dev_name;

	union 
	{
		struct fwk_pinctrl_mux sgrt_mux;				/*!< a group of pin mux */
		struct fwk_pinctrl_configs sgrt_configs;		/*!< a group of pin conf */
	} ugrt_data;

} srt_fwk_pinctrl_setting_t;

/*!< a state represents a pinctrl (pinctrl-0, pinctrl-1, ...) */
typedef struct fwk_pinctrl_state 
{
	struct list_head sgrt_link;                         /*!< pinctrl_state list, link to fwk_pinctrl::sgrt_states */
	const kstring_t *name;                              /*!< pinctrl-names */
	struct list_head sgrt_settings;                     /*!< list head of fwk_pinctrl_setting::sgrt_link */

} srt_fwk_pinctrl_state_t;

typedef struct fwk_pinctrl
{
	struct list_head sgrt_link;							/*!< link to global list */
	struct fwk_device *sprt_dev;						/*!< sprt_dev == fwk_pinctrl_dev::sprt_dev */
	struct list_head sgrt_states;                       /*!< list head of pinctrl_state::sgrt_link */
	struct fwk_pinctrl_state *sprt_state;               /*!< current usage state, such as "default" */
	struct list_head sgrt_dt_maps;

} srt_fwk_pinctrl_t;

/*!< The functions */
TARGET_EXT srt_fwk_pinctrl_dev_t *fwk_pinctrl_register(srt_fwk_pinctrl_desc_t *sprt_desc, srt_fwk_device_t *sprt_dev, void *driver_data);
TARGET_EXT void fwk_pinctrl_unregister(srt_fwk_pinctrl_dev_t *sprt_pctldev);
TARGET_EXT srt_fwk_pinctrl_t *fwk_pinctrl_get(srt_fwk_device_t *sprt_dev);
TARGET_EXT void fwk_pinctrl_put(srt_fwk_pinctrl_t *sprt_pctl);
TARGET_EXT srt_fwk_pinctrl_state_t *fwk_pinctrl_lookup_state(srt_fwk_pinctrl_t *sprt_pctl, const kstring_t *state_name);
TARGET_EXT ksint32_t fwk_pinctrl_select_state(srt_fwk_pinctrl_t *sprt_pctl, srt_fwk_pinctrl_state_t *sprt_state);
TARGET_EXT ksint32_t fwk_pinctrl_bind_pins(srt_fwk_device_t *sprt_dev);
TARGET_EXT void fwk_pinctrl_unbind_pins(srt_fwk_device_t *sprt_dev);

TARGET_EXT ksint32_t fwk_pinmux_map_to_setting(srt_fwk_pinctrl_map_t const *sprt_map, srt_fwk_pinctrl_setting_t *sprt_setting);
TARGET_EXT ksint32_t fwk_pinconf_get_by_name(srt_fwk_pinctrl_dev_t *sprt_pctldev, const kstring_t *name);
TARGET_EXT ksint32_t fwk_pinconf_map_to_setting(srt_fwk_pinctrl_map_t const *sprt_map, srt_fwk_pinctrl_setting_t *sprt_setting);

/*!< API functions */
static inline void *fwk_pinctrl_get_drvdata(srt_fwk_pinctrl_dev_t *sprt_pctldev)
{
	return sprt_pctldev->driver_data;
}

#endif /*!< __FWK_PINCTRL_H_ */
