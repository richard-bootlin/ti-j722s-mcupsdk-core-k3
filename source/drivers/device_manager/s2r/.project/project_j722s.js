let path = require('path');

const files_r5f = {
    common: [
        "lpm_s2r.c",
    ]
};

const includes_r5f = {
    common: [
        "soc/j722s",
	"./",
        "../../hw_include/j722s",
        "../../i2c/v0",
        "../rm_pm_hal/rm_pm_hal_src/lpm/cdns_generated_defines",
        "../rm_pm_hal/rm_pm_hal_src/lpm/include/soc/am62px",
        "../rm_pm_hal/rm_pm_hal_src/include",
        "../rm_pm_hal/rm_pm_hal_src",
    ],
};

const filedirs_r5f = {
    common: [
        ".",
        "soc/j722s",
    ],
};

const defines_r5f = {
    common: [
    ],
};

const cflags = {
    common: [
	    "-nostdlib",
	    "-fomit-frame-pointer",
	    "-fno-builtin",
	    "-Wno-unused-variable",
	    "-static",
    ],
    remove: [
        "-mthumb",
    ],
};

const buildOptionCombos = [
    { device: device, cpu: "wkup-r5f", cgt: "ti-arm-clang"},
];

function getComponentProperty() {
    let property = {};

    property.dirPath = path.resolve(__dirname, "..");
    property.type = "library";
    property.name = "s2r";
    property.isInternal = false;
    property.buildOptionCombos = buildOptionCombos;

    return property;
}

function getComponentBuildProperty(buildOption) {
    let build_property = {};

    if(buildOption.cpu == "wkup-r5f") {
        build_property.files = files_r5f;
        build_property.includes = includes_r5f;
        build_property.filedirs = filedirs_r5f;
        build_property.defines = defines_r5f;
        build_property.cflags = cflags;
    }

    return build_property;
}

module.exports = {
    getComponentProperty,
    getComponentBuildProperty,
};
