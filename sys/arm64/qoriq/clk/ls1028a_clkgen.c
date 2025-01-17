/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Alstom Group.
 * Copyright (c) 2021 Semihalf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/rman.h>
#include <machine/bus.h>

#include <dev/fdt/simplebus.h>

#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <dev/extres/clk/clk_fixed.h>

#include <arm64/qoriq/clk/qoriq_clkgen.h>

static uint8_t ls1028a_pltfrm_pll_divs[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0
};

static struct qoriq_clk_pll_def ls1028a_pltfrm_pll = {
	.clkdef = {
		.name = "ls1028a_platform_pll",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_PLATFORM_PLL, 0),
		.flags = 0
	},
	.offset = 0x60080,
	.shift = 1,
	.mask = 0xFE,
	.dividers = ls1028a_pltfrm_pll_divs,
	.flags = QORIQ_CLK_PLL_HAS_KILL_BIT
};

static const uint8_t ls1028a_cga_pll_divs[] = {
	2, 3, 4, 0
};

static struct qoriq_clk_pll_def ls1028a_cga_pll1 = {
	.clkdef = {
		.name = "ls1028a_cga_pll1",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_INTERNAL, 0),
		.flags = 0
	},
	.offset = 0x80,
	.shift = 1,
	.mask = 0xFE,
	.dividers = ls1028a_cga_pll_divs,
	.flags = QORIQ_CLK_PLL_HAS_KILL_BIT
};

static struct qoriq_clk_pll_def ls1028a_cga_pll2 = {
	.clkdef = {
		.name = "ls1028a_cga_pll2",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_INTERNAL, 20),
		.flags = 0
	},
	.offset = 0xA0,
	.shift = 1,
	.mask = 0xFE,
	.dividers = ls1028a_cga_pll_divs,
	.flags = QORIQ_CLK_PLL_HAS_KILL_BIT
};

static struct qoriq_clk_pll_def *ls1028a_cga_plls[] = {
	&ls1028a_cga_pll1,
	&ls1028a_cga_pll2
};

static const char *ls1028a_cmux0_parent_names[] = {
	"ls1028a_cga_pll1",
	"ls1028a_cga_pll1_div2",
	"ls1028a_cga_pll1_div4",
	NULL,
	"ls1028a_cga_pll2",
	"ls1028a_cga_pll2_div2",
	"ls1028a_cga_pll2_div4"
};

static struct clk_mux_def ls1028a_cmux0 = {
	.clkdef = {
		.name = "ls1028a_cmux0",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_CMUX, 0),
		.parent_names = ls1028a_cmux0_parent_names,
		.parent_cnt = nitems(ls1028a_cmux0_parent_names),
		.flags = 0
	},
	.offset = 0x70000,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_cmux1 = {
	.clkdef = {
		.name = "ls1028a_cmux1",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_CMUX, 1),
		.parent_names = ls1028a_cmux0_parent_names,
		.parent_cnt = nitems(ls1028a_cmux0_parent_names),
		.flags = 0
	},
	.offset = 0x70020,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_cmux2 = {
	.clkdef = {
		.name = "ls1028a_cmux2",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_CMUX, 2),
		.parent_names = ls1028a_cmux0_parent_names,
		.parent_cnt = nitems(ls1028a_cmux0_parent_names),
		.flags = 0
	},
	.offset = 0x70040,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_cmux3 = {
	.clkdef = {
		.name = "ls1028a_cmux3",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_CMUX, 3),
		.parent_names = ls1028a_cmux0_parent_names,
		.parent_cnt = nitems(ls1028a_cmux0_parent_names),
		.flags = 0
	},
	.offset = 0x70060,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static const char *ls1028a_hwaccel1_parent_names[] = {
	"ls1028a_platform_pll",
	"ls1028a_cga_pll1",
	"ls1028a_cga_pll1_div2",
	"ls1028a_cga_pll1_div3",
	"ls1028a_cga_pll1_div4",
	NULL,
	"ls1028a_cga_pll2_div2",
	"ls1028a_cga_pll2_div3"
};

static const char *ls1028a_hwaccel2_parent_names[] = {
	"ls1028a_platform_pll",
	"ls1028a_cga_pll2",
	"ls1028a_cga_pll2_div2",
	"ls1028a_cga_pll2_div3",
	"ls1028a_cga_pll2_div4",
	NULL,
	"ls1028a_cga_pll1_div2",
	"ls1028a_cga_pll1_div3"
};

static struct clk_mux_def ls1028a_hwaccel1 = {
	.clkdef = {
		.name = "ls1028a_hwaccel1",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_HWACCEL, 0),
		.parent_names = ls1028a_hwaccel1_parent_names,
		.parent_cnt = nitems(ls1028a_hwaccel1_parent_names),
		.flags = 0
	},
	.offset = 0x10,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_hwaccel2 = {
	.clkdef = {
		.name = "ls1028a_hwaccel2",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_HWACCEL, 1),
		.parent_names = ls1028a_hwaccel2_parent_names,
		.parent_cnt = nitems(ls1028a_hwaccel2_parent_names),
		.flags = 0
	},
	.offset = 0x30,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_hwaccel3 = {
	.clkdef = {
		.name = "ls1028a_hwaccel3",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_HWACCEL, 2),
		.parent_names = ls1028a_hwaccel1_parent_names,
		.parent_cnt = nitems(ls1028a_hwaccel1_parent_names),
		.flags = 0
	},
	.offset = 0x50,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def ls1028a_hwaccel4 = {
	.clkdef = {
		.name = "ls1028a_hwaccel4",
		.id = QORIQ_CLK_ID(QORIQ_TYPE_HWACCEL, 3),
		.parent_names = ls1028a_hwaccel2_parent_names,
		.parent_cnt = nitems(ls1028a_hwaccel2_parent_names),
		.flags = 0
	},
	.offset = 0x70,
	.shift = 27,
	.width = 4,
	.mux_flags = 0
};

static struct clk_mux_def *ls1028a_mux_nodes[] = {
	&ls1028a_cmux0,
	&ls1028a_cmux1,
	&ls1028a_cmux2,
	&ls1028a_cmux3,
	&ls1028a_hwaccel1,
	&ls1028a_hwaccel2,
	&ls1028a_hwaccel3,
	&ls1028a_hwaccel4
};

static int ls1028a_clkgen_probe(device_t);
static int ls1028a_clkgen_attach(device_t);

static device_method_t ls1028a_clkgen_methods[] = {
	DEVMETHOD(device_probe,		ls1028a_clkgen_probe),
	DEVMETHOD(device_attach,	ls1028a_clkgen_attach),

	DEVMETHOD_END
};

DEFINE_CLASS_1(ls1028a_clkgen, ls1028a_clkgen_driver, ls1028a_clkgen_methods,
    sizeof(struct qoriq_clkgen_softc), qoriq_clkgen_driver);

EARLY_DRIVER_MODULE(ls1028a_clkgen, simplebus, ls1028a_clkgen_driver, 0, 0,
    BUS_PASS_BUS + BUS_PASS_ORDER_MIDDLE);

static int
ls1028a_clkgen_probe(device_t dev)
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if(!ofw_bus_is_compatible(dev, "fsl,ls1028a-clockgen"))
		return (ENXIO);

	device_set_desc(dev, "LS1028A clockgen");
	return (BUS_PROBE_DEFAULT);
}

static int
ls1028a_clkgen_attach(device_t dev)
{
	struct qoriq_clkgen_softc *sc;

	sc = device_get_softc(dev);

	sc->pltfrm_pll_def = &ls1028a_pltfrm_pll;
	sc->cga_pll = ls1028a_cga_plls;
	sc->cga_pll_num = nitems(ls1028a_cga_plls);
	sc->mux = ls1028a_mux_nodes;
	sc->mux_num = nitems(ls1028a_mux_nodes);
	sc->flags = QORIQ_LITTLE_ENDIAN;

	return (qoriq_clkgen_attach(dev));
}
