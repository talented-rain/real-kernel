/*
 * Hardware Abstraction Layer Pinctrl Interface
 *
 * File Name:   fwk_pinmux.c
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2024.04.22
 *
 * Copyright (c) 2024   Yang Yujun <yujiantianhu@163.com>
 *
 */

/*!< The includes */
#include <platform/fwk_pinctrl.h>

/*!< The defines */

/*!< The globals */

/*!< API function */
/*!
 * @brief   map sprt_map(type = mux) to sprt_setting
 * @param   sprt_pctldev: pinctrl_dev
 * @param   name: function name (it may be the name of "iomuxc" device_node)
 * @retval  error code
 * @note    none
 */
static kint32_t fwk_pinmux_get_function_by_name(struct fwk_pinctrl_dev *sprt_pctldev, const kchar_t *name)
{
    const struct fwk_pinmux_ops *sprt_pmxops;
    const kchar_t *func_name;
    kuint32_t i, num_funcs;

    sprt_pmxops = sprt_pctldev->sprt_desc->sprt_pmxops;
    if (!sprt_pmxops->get_functions_count ||
        !sprt_pmxops->get_function_name)
        return -ER_NODEV;

    num_funcs = sprt_pmxops->get_functions_count(sprt_pctldev);

    for (i = 0; i < num_funcs; i++)
    {
        func_name = sprt_pmxops->get_function_name(sprt_pctldev, i);
        if (!func_name)
            continue;

        if (!strcmp(name, func_name))
            return i;
    }

    return -ER_MORE;
}

/*!
 * @brief   map sprt_map(type = mux) to sprt_setting
 * @param   sprt_pctldev: pinctrl_dev
 * @param   func_selector: function index
 * @param   name: group name (pinctrl-0/1/2, ..., phandle ---> device node)
 * @retval  error code
 * @note    none
 */
static kint32_t fwk_pinmux_get_group_by_name(struct fwk_pinctrl_dev *sprt_pctldev, kuint32_t func_selector, const kchar_t *name)
{
    const struct fwk_pinctrl_ops *sprt_pctlops;
    const kchar_t *grp_name;
    kuint32_t i, num_grps;

    sprt_pctlops = sprt_pctldev->sprt_desc->sprt_pctlops;
    if (!sprt_pctlops->get_groups_count ||
        !sprt_pctlops->get_group_name)
        return -ER_NODEV;

    /*!< how many groups of this functions */
    num_grps = sprt_pctlops->get_groups_count(sprt_pctldev, func_selector);

    for (i = 0; i < num_grps; i++)
    {
        grp_name = sprt_pctlops->get_group_name(sprt_pctldev, func_selector, i);
        if (!grp_name)
            continue;

        if (!strcmp(name, grp_name))
            return i;
    }

    return -ER_MORE;
}

/*!
 * @brief   map sprt_map(type = mux) to sprt_setting
 * @param   sprt_map: current map (source data)
 * @param   sprt_setting: destination data
 * @retval  error code
 * @note    none
 */
kint32_t fwk_pinmux_map_to_setting(struct fwk_pinctrl_map const *sprt_map, struct fwk_pinctrl_setting *sprt_setting)
{
    struct fwk_pinctrl_dev *sprt_pctldev;
    kint32_t func_selector, group_selector;

    if (sprt_setting->type != NR_FWK_PINCTRL_PIN_MUX)
        return -ER_FAULT;

    sprt_pctldev = sprt_setting->sprt_pctldev;

    /*!< get function index */
    func_selector = fwk_pinmux_get_function_by_name(sprt_pctldev, sprt_map->ugrt_data.sgrt_mux.function);
    if (func_selector < 0)
        return func_selector;

    /*!< get group index */
    group_selector = fwk_pinmux_get_group_by_name(sprt_pctldev, func_selector, sprt_map->ugrt_data.sgrt_mux.group);
    if (group_selector < 0)
        return group_selector;

    sprt_setting->ugrt_data.sgrt_mux.func = func_selector;
    sprt_setting->ugrt_data.sgrt_mux.group = group_selector;

    return ER_NORMAL;
}

/*!< end of file */
