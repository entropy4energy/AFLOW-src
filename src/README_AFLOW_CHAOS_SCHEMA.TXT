{
  "__schema^2__": {
    "__comment__": [
      "The zeroth element of any object or array in this document is meta.",
      "If last element is null, element parent considered optional.",
      "If last element is '.', element value can be anything.",
      "If last element is '', element value can be nothing.",
      "This document is the AAPI schema, it is self validating and order sensitive.",
      "."
    ],
    "correlated": [
      "For correlated properties, lists the property dependencies.",
      "The counts of the data have to match.",
      "."
    ],
    "delimiter": [
      "An ordered set of single character seperators for distinguishing plural type property values",
      null
    ],
    "description": [
      "intended for popup help boxes, describes the current property: freeform text",
      "."
    ],
    "example": [
      "Actual result that may occur in API or search context, developmental: structured text",
      "."
    ],
    "expression": [
      "intended for materials reports, developmental. Must be one of",
      "correlated",
      "declarative",
      "directive",
      "derivative"
    ],
    "format": [
      "intended for printf style formating of property value: corresponds to the type attribute",
      "."
    ],
    "function": [
      "defines the post-processing function that is used by AFLOW.",
      "if array, the functions will be resolved sequentially. Must be one of",
      "atom_resolved",
      "compound",
      "crystallography",
      "icsd",
      "ldau",
      "params",
      "spacegroup",
      "species_resolved",
      null
    ],
    "inclusion": [
      "intended for search filters and materials reports. Optional entries are only shown when not null, mandatory entries are always shown. Must be one of",
      "mandatory",
      "conditional",
      "optional",
      "forbidden"
    ],
    "references": [
      "the publications associated with the property",
      ""
    ],
    "search": [
      [
        "intended for search and stat, Must be one of",
        "equals -> exact match input (select or freeform) to value",
        "contains -> substring match (select or freeform) in value",
        "range ->   bounded match (select or freeform) in value"
      ],
      "equals",
      "contains",
      "range",
      null
    ],
    "status": [
      "Development stage of property. Must be one of",
      "production",
      "development",
      "deprecated",
      "reserved"
    ],
    "syntax": [
      "Actual setting that may be used in API or search context, developmental: structured text",
      "."
    ],
    "title": [
      "intended for labeling property in document rendering: freeform text (HTML?)",
      "."
    ],
    "title_short": [
      "intended for labeling property in document rendering when space is limited",
      "."
    ],
    "type": [
      "intended for DB and document type handling: must be one of",
      "string",
      "strings",
      "number",
      "numbers"
    ],
    "units": [
      "units for search filter number in HTML: optional",
      null
    ],
    "verification": [
      "Optional list of property references designed to certify that the result is contextually relevant.",
      null
    ]
  },
  "ael_applied_pressure": {
    "__comment__": [
      ""
    ],
    "description": "Returns the applied pressure for the AEL calculations.",
    "title": "applied pressure (AEL)",
    "format": "%.3g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_applied_pressure=0.0",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_applied_pressure"
  },
  "ael_average_external_pressure": {
    "__comment__": [
      ""
    ],
    "description": "Returns the average external pressure for the AEL calculations.",
    "title": "AEL average external pressure",
    "format": "%.2g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_average_external_pressure=0.079875",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_average_external_pressure"
  },
  "ael_bulk_modulus_reuss": {
    "__comment__": [
      ""
    ],
    "description": "Returns the bulk modulus calculated, using the Reuss method, by AEL.",
    "title": "AEL bulk modulus Reuss",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_bulk_modulus_reuss=105.315",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_bulk_modulus_reuss"
  },
  "ael_bulk_modulus_voigt": {
    "__comment__": [
      ""
    ],
    "description": "Returns the bulk modulus calculated, using the Voigt method, by AEL.",
    "title": "AEL bulk modulus Voigt",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_bulk_modulus_voiht=105.315",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_bulk_modulus_voigt"
  },
  "ael_bulk_modulus_vrh": {
    "__comment__": [
      ""
    ],
    "description": "Returns the bulk modulus calculated, using the Voigt-Reuss-Hill average, by AEL.",
    "title": "AEL bulk modulus VRH",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_bulk_modulus_vrh=105.315",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_bulk_modulus_vrh"
  },
  "ael_compliance_tensor": {
    "__comment__": [],
    "description": "Returns the compliance tensor calculated by AEL.",
    "title": "Compliance tensor",
    "format": "%.4g",
    "function": "matrix",
    "type": "numbers",
    "units": "GPa<sup>-1</sup>",
    "inclusion": "optional",
    "example": "74.2667,49.5167,49.5167,0,0,0;49.5167,74.2667,49.5167,0,0,0;49.5167,49.5167,74.2667,0,0,0;0,0,0,27.2833,0,0;0,0,0,0,27.2833,0;0,0,0,0,0,27.2833",
    "expression": "derivative",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_compliance_tensor"
  },
  "ael_debye_temperature": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Debye temperature calculated by AEL.",
    "title": "AEL Debye temperature",
    "format": "%.0f",
    "type": "number",
    "units": "K",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_debye_temperature=163.348",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_debye_temperature"
  },
  "ael_elastic_anisotropy": {
    "__comment__": [
      ""
    ],
    "description": "Returns the elastic anisotropy calculated by AEL.",
    "title": "AEL elastic anisotropy",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_elastic_anisotropy=0.0008165",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_elastic_anisotropy"
  },
  "ael_poisson_ratio": {
    "__comment__": [
      ""
    ],
    "description": "Returns the isotropic Poisson ratio calculated by AEL.",
    "title": "AEL Poisson ratio",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_poisson_ratio=0.216",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_poisson_ratio"
  },
  "ael_pughs_modulus_ratio": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pugh's modulus ratio calculated by AEL.",
    "title": "AEL Pugh's modulus ratio",
    "format": "%.4g",
    "type": "number",
    "units": "",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_pughs_modulus_ratio=0.332265",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_pughs_modulus_ratio"
  },
  "ael_shear_modulus_reuss": {
    "__comment__": [
      ""
    ],
    "description": "Returns the shear modulus calculated, using the Reuss method, by AEL.",
    "title": "AEL shear modulus Reuss",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_shear_modulus_reuss=73.787",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_shear_modulus_reuss"
  },
  "ael_shear_modulus_voigt": {
    "__comment__": [
      ""
    ],
    "description": "Returns the shear modulus calculated, using the Voigt method, by AEL.",
    "title": "AEL shear modulus Voigt",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_shear_modulus_voigt=73.799",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_shear_modulus_voigt"
  },
  "ael_shear_modulus_vrh": {
    "__comment__": [
      ""
    ],
    "description": "Returns the shear modulus calculated, using the Voigt-Reuss-Hill average, by AEL.",
    "title": "AEL shear modulus VRH",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_shear_modulus_vrh=73.793",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_shear_modulus_vrh"
  },
  "ael_speed_of_sound_average": {
    "__comment__": [
      ""
    ],
    "description": "Returns the average speed of sound calculated by AEL",
    "title": "AEL speed of sound average",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_of_sound_average=1540.09",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?ael_speed_of_sound_average"
  },
  "ael_speed_of_sound_longitudinal": {
    "__comment__": [
      ""
    ],
    "description": "Returns the longitudinal speed of sound calculated by AEL",
    "title": "AEL speed of sound longitudinal",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_of_sound_longitudinal=2147.05",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?ael_speed_of_sound_longitudinal"
  },
  "ael_speed_of_sound_transverse": {
    "__comment__": [
      ""
    ],
    "description": "Returns the transverse speed of sound calculated by AEL",
    "title": "AEL speed of sound transverse",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_of_sound_transverse=1405.57",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?ael_speed_of_sound_transverse"
  },
  "ael_speed_sound_average": {
    "__comment__": [
      ""
    ],
    "description": "Returns the average speed of sound calculated by AEL",
    "title": "AEL speed of sound average",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_sound_average=1540.09",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_speed_sound_average"
  },
  "ael_speed_sound_longitudinal": {
    "__comment__": [
      ""
    ],
    "description": "Returns the longitudinal speed of sound calculated by AEL",
    "title": "AEL speed of sound longitudinal",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_sound_longitudinal=2147.05",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_speed_sound_longitudinal"
  },
  "ael_speed_sound_transverse": {
    "__comment__": [
      ""
    ],
    "description": "Returns the transverse speed of sound calculated by AEL",
    "title": "AEL speed of sound transverse",
    "format": "%.0f",
    "type": "number",
    "units": "m/s",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_speed_sound_transverse=1405.57",
    "references": [
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_speed_sound_transverse"
  },
  "ael_stiffness_tensor": {
    "__comment__": [],
    "description": "Returns the stiffness tensor calculated by AEL",
    "title": "AEL stiffness tensor",
    "format": "%.4g",
    "function": "matrix",
    "type": "numbers",
    "units": "GPa",
    "inclusion": "optional",
    "example": "0.0288595,-0.0115446,-0.0115446,0,0,0;-0.0115446,0.0288595,-0.0115446,0,0,0;-0.0115446,-0.0115446,0.0288595,0,0,0;0,0,0,0.0366524,0,0;0,0,0,0,0.0366524,0;0,0,0,0,0,0.0366524",
    "expression": "derivative",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_stiffness_tensor"
  },
  "ael_youngs_modulus_vrh": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Young modulus calculated, using the Voigt-Reuss-Hill average, by AEL.",
    "title": "AEL Young's modulus VRH",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "ael_youngs_modulus_vrh=31.6223",
    "references": [
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?ael_youngs_modulus_vrh"
  },
  "aflow_prototype_label_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype label for the unrelaxed structure.",
    "title": "AFLOW prototype label unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_label_orig=A2BC4_cF56_227_c_b_e",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_label_orig"
  },
  "aflow_prototype_label_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype label for the relaxed structure.",
    "title": "AFLOW prototype label relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_label_relax=A2BC4_cF56_227_c_b_e",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_label_relax"
  },
  "aflow_prototype_parameters_relax": {
    "description": "Returns the AFLOW prototype parameter labels and values for the relaxed structure.",
    "title": "AFLOW prototype parameters relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "correlated",
    "status": "production",
    "correlated": [
      "aflow_prototype_params_list_relax",
      "aflow_prototype_params_values_relax"
    ]
  },
  "aflow_prototype_parameters_orig": {
    "description": "Returns the AFLOW prototype parameter labels and values for the unrelaxed structure.",
    "title": "AFLOW prototype parameters unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "correlated",
    "status": "production",
    "correlated": [
      "aflow_prototype_params_list_orig",
      "aflow_prototype_params_values_orig"
    ]
  },
  "aflow_prototype_params_list_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype parameter labels for the relaxed structure.",
    "title": "AFLOW prototype parameter label relaxed",
    "title_short": "parameter",
    "format": "%s",
    "function": "params",
    "type": "strings",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_params_list=a,x3",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_params_list"
  },
  "aflow_prototype_params_list_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype parameter labels for the unrelaxed structure.",
    "title": "AFLOW prototype parameter label unrelaxed",
    "title_short": "parameter",
    "format": "%s",
    "function": "params",
    "type": "strings",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_params_list_orig=a,x3",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_params_list_orig"
  },
  "aflow_prototype_params_values_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype parameter values for the relaxed structure.",
    "title": "AFLOW prototype parameter values relaxed",
    "title_short": "value",
    "format": "%g",
    "type": "numbers",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_params_list=10.336,0.757",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_params_values"
  },
  "aflow_prototype_params_values_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype parameter values for the unrelaxed structure.",
    "title": "AFLOW prototype parameter values unrelaxed",
    "title_short": "value",
    "format": "%g",
    "type": "numbers",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aflow_prototype_params_values_orig=10.336,0.757",
    "references": [
      "10.1038/s41524-020-00483-4"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_prototype_params_values_orig"
  },
  "aflow_version": {
    "__comment__": [
      ""
    ],
    "description": "Returns the version number of AFLOW used to perform the calculation.",
    "title": "AFLOW version",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "aflow_version=aflow30641",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?aflow_version"
  },
  "aflowlib_date": {
    "__comment__": [
      ""
    ],
    "description": "Returns the date when the AFLOW post-processor generated the entry in the library.",
    "title": "AFLOWLIB date",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "aflowlib_date=20140204_13:10:39_GMT-5",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?aflowlib_date"
  },
  "aflowlib_entries": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOWLIB entries that matched the search criterion.",
    "title": "AFLOWLIB entries",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "conditional",
    "expression": "directive",
    "example": "aflowlib_entries=AgAl,AgAs,AgAu,AgB_h,AgBa_sv,AgBe_sv,AgBi_d,AgBr,AgCa_sv,...",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?aflowlib_entries"
  },
  "aflowlib_entries_number": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number AFLOWLIB entries that matched the search criterion.",
    "title": "AFLOWLIB entry count",
    "format": "%d",
    "type": "number",
    "inclusion": "conditional",
    "expression": "directive",
    "example": "aflowlib_entries_number=654",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?aflowlib_entries_number"
  },
  "aflowlib_version": {
    "__comment__": [
      ""
    ],
    "description": "Returns the version of the AFLOW post-processor which generated the entry in the library.",
    "title": "AFLOWLIB version",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "aflowlib_version=3.1.103",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?aflowlib_version"
  },
  "agl_acoustic_debye": {
    "__comment__": [
      ""
    ],
    "description": "Returns the acoustic Debye temperature calculated by AGL.",
    "title": "AGL Debye temperature acoustic",
    "format": "%.0f",
    "type": "number",
    "units": "K",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_acoustic_debye=492",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_acoustic_debye"
  },
  "agl_bulk_modulus_isothermal_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the isothermal bulk modulus calculated by AGL at 300 K.",
    "title": "AGL bulk modulus isothermal (300 K)",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_bulk_modulus_isothermal_300K=96.6",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_bulk_modulus_isothermal_300K"
  },
  "agl_bulk_modulus_static_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the static bulk modulus calculated by AGL at 300 K.",
    "title": "AGL bulk modulus static (300 K)",
    "format": "%.4g",
    "type": "number",
    "units": "GPa",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_bulk_modulus_static_300K=99.6",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_bulk_modulus_static_300K"
  },
  "agl_debye": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Debye temperature calculated by AGL.",
    "title": "AGL Debye temperature",
    "format": "%.0f",
    "type": "number",
    "units": "K",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_debye=620",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_debye"
  },
  "agl_gruneisen": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Gr&uuml;neisen parameter calculated by AGL.",
    "title": "AGL Gr&uuml;neisen parameter",
    "format": "%.3g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_gruneisen=2.06",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_gruneisen"
  },
  "agl_heat_capacity_Cp_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the heat capacity per cell, at constant pressure, calculated by AGL at 300 K.",
    "title": "AGL heat capacity constant pressure (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "k<sub>B</sub>/cell",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_heat_capacity_Cp_300K=5.502",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_heat_capacity_Cp_300K"
  },
  "agl_heat_capacity_Cv_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the heat capacity per cell, at constant volume, calculated by AGL at 300 K.",
    "title": "AGL heat capacity constant volume (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "k<sub>B</sub>/cell",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_heat_capacity_Cv_300K=4.901",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_heat_capacity_Cv_300K"
  },
  "agl_poisson_ratio_source": {
    "__comment__": [
      ""
    ],
    "description": "Returns the source of the Poisson ratio used for AGL calculations.",
    "title": "AGL Poisson ratio source",
    "format": "%s",
    "type": "string",
    "units": "",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_poisson_ratio_source=ael_poisson_ratio_0.350433",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_poisson_ratio_source"
  },
  "agl_thermal_conductivity_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the thermal conductivity calculated by AGL at 300 K.",
    "title": "AGL thermal conductivity (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "W m<sup>-1</sup> K<sup>-1</sup>",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_thermal_conductivity_300K=24.41",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_thermal_conductivity_300K"
  },
  "agl_thermal_expansion_300K": {
    "__comment__": [
      ""
    ],
    "description": "Returns the thermal expansion coefficient calculated by AGL at 300 K.",
    "title": "AGL thermal expansion coefficient (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "K<sup>-1</sup>",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_thermal_expansion_300K=4.997e-05",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_thermal_expansion_300K"
  },
  "agl_vibrational_entropy_300K_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the vibrational entropy per atom calculated by AGL at 300 K.",
    "title": "AGL vibrational entropy atom (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "meV/(K atom)",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_vibrational_entropy_300K_atom=0.454761",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_vibrational_entropy_300K_atom"
  },
  "agl_vibrational_entropy_300K_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the vibrational entropy per cell calculated by AGL at 300 K.",
    "title": "AGL vibrational entropy cell (300 K)",
    "format": "%.3g",
    "type": "number",
    "units": "meV/(K cell)",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_vibrational_entropy_300K_cell=9.09521",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_vibrational_entropy_300K_cell"
  },
  "agl_vibrational_free_energy_300K_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the vibrational free energy per atom calculated by AGL at 300 K.",
    "title": "AGL vibrational free energy atom (300 K)",
    "format": "%.1f",
    "type": "number",
    "units": "meV/atom",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_vibrational_free_energy_300K_atom=-57.1897",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_vibrational_free_energy_300K_atom"
  },
  "agl_vibrational_free_energy_300K_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the vibrational free energy per cell calculated by AGL at 300 K.",
    "title": "AGL vibrational free energy cell (300 K)",
    "format": "%.1f",
    "type": "number",
    "units": "meV/cell",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "agl_vibrational_free_energy_300K_cell=-1143.79",
    "references": [
      "10.1103/PhysRevB.90.174107",
      "10.1103/PhysRevMaterials.1.015401"
    ],
    "status": "production",
    "syntax": "$aurl/?agl_vibrational_free_energy_300K_cell"
  },
  "auid": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOWLIB unique identifier (AUID) for the entry.",
    "title": "AFLOWLIB unique identifier",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "auid=aflow:e9c6d914c4b8d9ca",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?auid"
  },
  "aurl": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOWLIB uniform resource locator (AURL) for the entry.",
    "title": "AFLOWLIB uniform resource locator",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "aurl=s4e.ai:AFLOWDATA/LIB3_RAW/Bi_dRh_pvTi_sv/T0003.ABC:LDAU2",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?aurl"
  },
  "author": {
    "__comment__": [
      ""
    ],
    "description": "Returns the name (not necessarily an individual) and affiliation associated with authorship of the data.",
    "title": "author",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "author=Marco_Buongiorno_Nardelli,Ohad_Levy,Jesus_Carrete",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?author"
  },
  "bader_atomic_volumes": {
    "__comment__": [
      ""
    ],
    "description": "Returns the volume of each atom calculated by the Atoms in Molecules (AIM) Bader analysis.",
    "title": "Bader atom volume",
    "format": "%.2f",
    "function": "atom_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "&Aring;<sup>3</sup>",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "bader_atomic_volumes=15.235,12.581,13.009",
    "references": [],
    "status": "production",
    "syntax": "$aurl/?bader_atomic_volumes"
  },
  "bader_net_charges": {
    "__comment__": [
      ""
    ],
    "description": "Returns the partial charge of each atom calculated by the Atoms in Molecules (AIM) Bader analysis.",
    "title": "Bader atom charge",
    "format": "%.2f",
    "function": "atom_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "e<sup>-</sup>",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "bader_net_charges=0.125,0.125,-0.25",
    "references": [],
    "status": "production",
    "syntax": "$aurl/?bader_net_charges"
  },
  "Bravais_lattice_lattice_system": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice of the lattice system for the relaxed structure.",
    "title": "Bravais lattice of lattice system relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_system=cubic",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_lattice_system"
  },
  "Bravais_lattice_lattice_system_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice of the lattice system for the unrelaxed structure.",
    "title": "Bravais lattice of lattice system unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_system=cubic",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_lattice_system_orig"
  },
  "Bravais_lattice_lattice_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice centering type for the relaxed structure.",
    "title": "Bravais lattice centering type relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_type=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_lattice_type"
  },
  "Bravais_lattice_lattice_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice centering type for the unrelaxed structure.",
    "title": "Bravais lattice centering type unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_type_orig=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_lattice_type_orig"
  },
  "Bravais_lattice_lattice_variation_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice variation of the lattice system for the relaxed structure.",
    "title": "Bravais lattice of lattice system variation relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_variation_type=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_lattice_variation_type"
  },
  "Bravais_lattice_lattice_variation_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice variation of the lattice system for the unrelaxed structure.",
    "title": "Bravais lattice of lattice system variation unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_lattice_variation_type_orig=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_variation_lattice_type_orig"
  },
  "Bravais_lattice_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice of the crystal for the unrelaxed structure.",
    "title": "Bravais lattice of crystal unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_lattice_orig=MCLC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_orig"
  },
  "Bravais_lattice_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais lattice of the crystal for the relaxed structure.",
    "title": "Bravais lattice of crystal relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "Bravais_lattice_relax=MCLC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_lattice_relax"
  },
  "Bravais_superlattice_lattice_system": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice of the lattice system for the relaxed structure.",
    "title": "Bravais superlattice of lattice system relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_system=cubic",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_lattice_system"
  },
  "Bravais_superlattice_lattice_system_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice of the lattice system for the unrelaxed structure.",
    "title": "Bravais superlattice of lattice system unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_system=cubic",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_lattice_system_orig"
  },
  "Bravais_superlattice_lattice_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice centering type for the relaxed structure.",
    "title": "Bravais superlattice centering type relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_type=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_lattice_type"
  },
  "Bravais_superlattice_lattice_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice centering type for the unrelaxed structure.",
    "title": "Bravais superlattice centering type unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_type_orig=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_lattice_type_orig"
  },
  "Bravais_superlattice_lattice_variation_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice variation of the lattice system for the relaxed structure.",
    "title": "Bravais superlattice of lattice system variation relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_variation_type=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_lattice_variation_type"
  },
  "Bravais_superlattice_lattice_variation_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Bravais superlattice variation of the lattice system for the unrelaxed structure",
    "title": "Bravais superlattice of lattice system variation unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Bravais_superlattice_lattice_variation_type_orig=BCC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Bravais_superlattice_variation_lattice_type_orig"
  },
  "calculation_cores": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of CPUs used by the calculation.",
    "title": "used CPUs",
    "format": "%d",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "calculation_cores=32",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?calculation_cores"
  },
  "calculation_memory": {
    "__comment__": [
      ""
    ],
    "description": "Returns the maximum RAM used by the calculation.",
    "title": "used RAM",
    "format": "%.0f",
    "type": "number",
    "units": "MB",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "calculation_memory=32",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?calculation_memory"
  },
  "calculation_time": {
    "__comment__": [
      ""
    ],
    "description": "Returns the total time taken by the calculation.",
    "title": "used time",
    "format": "%d",
    "type": "number",
    "units": "seconds",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "calculation_time=32",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?calculation_time"
  },
  "catalog": {
    "__comment__": [
      ""
    ],
    "description": "Returns the database name for the calculation.",
    "title": "catalog",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "catalog=icsd",
    "references": [
      "10.1016/j.commatsci.2012.02.002"
    ],
    "status": "production",
    "syntax": "$aurl/?catalog"
  },
  "code": {
    "__comment__": [
      ""
    ],
    "description": "Returns the software name and version used to perform the calculation.",
    "title": "ab initio code",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "code=vasp.4.6.35",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?code"
  },
  "composition": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of atoms per type in the simulation cell.",
    "title": "composition",
    "format": "%d",
    "type": "numbers",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "composition=2,6,6",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?composition"
  },
  "compound": {
    "__comment__": [
      ""
    ],
    "description": "Returns the chemical formula of the structure.",
    "title": "chemical formula",
    "format": "%s",
    "function": "compound",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "compound=Co2Er6Si6",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?compound"
  },
  "corresponding": {
    "__comment__": [
      ""
    ],
    "description": "Returns the name (not necessarily an individual) and affiliation associated with the data origin concerning correspondence about data.",
    "title": "coresponding",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "corresponding=M_Buongiorno_Nardelli_mbn@unt.edu",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?corresponding"
  },
  "crystal_class": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal class for the relaxed structure.",
    "title": "crystal class relaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_class=tetrahedral",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_class"
  },
  "crystal_class_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal class for the unrelaxed structure.",
    "title": "crystal class unrelaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_class_orig=tetrahedral",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_class_orig"
  },
  "crystal_family": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal family for the relaxed structure.",
    "title": "crystal family relaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_family=cubic",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_family"
  },
  "crystal_family_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal family for the unrelaxed structure.",
    "title": "crystal family unrelaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_family_orig=cubic",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_family_orig"
  },
  "crystal_system": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal system for the relaxed structure.",
    "title": "crystal system relaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_system=cubic",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_system"
  },
  "crystal_system_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the crystal system for the unrelaxed structure.",
    "title": "crystal system unrelaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "crystal_system_orig=cubic",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?crystal_system_orig"
  },
  "data_api": {
    "__comment__": [
      ""
    ],
    "description": "Returns the REST API version for the entry.",
    "title": "REST API version",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "data_api=aapi1.0",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?data_api"
  },
  "data_language": {
    "__comment__": [
      ""
    ],
    "description": "Gives the language of the data in AFLOWLIB.",
    "title": "data language",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "data_language=aflowlib",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?data_language"
  },
  "data_source": {
    "__comment__": [
      ""
    ],
    "description": "Returns the data source for the entry.",
    "title": "data source",
    "format": "%s",
    "type": "string",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "data_source=aflowlib",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?data_source"
  },
  "delta_electronic_energy_convergence": {
    "__comment__": [
      ""
    ],
    "description": "Returns the change in total energy from the last step of the self-consistent field (SCF) iteration.",
    "title": "energy SCF change final",
    "format": "%.3e",
    "type": "number",
    "units": "eV",
    "inclusion": "optional",
    "expression": "derivative",
    "example": "delta_electronic_energy_convergence=6.09588e-05",
    "references": [
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?delta_electronic_energy_convergence"
  },
  "delta_electronic_energy_threshold": {
    "__comment__": [
      ""
    ],
    "description": "Returns the threshold for the self-consistent field (SCF) convergence.",
    "title": "energy SCF threshold",
    "format": "%e",
    "type": "number",
    "units": "eV",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "delta_electronic_energy_threshold=0.0001",
    "references": [
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?delta_electronic_energy_threshold"
  },
  "density": {
    "__comment__": [
      ""
    ],
    "description": "Returns the mass density of the unit cell.",
    "title": "mass density",
    "format": "%.4g",
    "type": "number",
    "units": "g/cm<sup>3</sup>",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "density=7.76665",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?density"
  },
  "dft_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the level of theory used in the calculation, i.e., pseudopotential type, exchange-correlation functional used, and use of GW.",
    "title": "DFT type",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "dft_type=PAW_PBE,HSE06",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?dft_type"
  },
  "eentropy_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the electronic entropy per atom used to converge the calculation.",
    "title": "electronic entropy atom",
    "format": "%g",
    "type": "number",
    "units": "eV/atom",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "eentropy_atom=0.0011",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?eentropy_atom"
  },
  "eentropy_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the electronic entropy per cell used to converge the calculation.",
    "title": "electronic entropy cell",
    "format": "%g",
    "type": "number",
    "units": "eV/cell",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "eentropy_cell=0.0011",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?eentropy_cell"
  },
  "Egap": {
    "__comment__": [
      ""
    ],
    "description": "Returns the electronic band gap.",
    "title": "electronic band gap",
    "format": "%.3g",
    "type": "number",
    "units": "eV",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "Egap=2.5",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?Egap"
  },
  "Egap_fit": {
    "__comment__": [
      ""
    ],
    "description": "Simple cross-validated correction (fit) of Egap.",
    "title": "fitted band gap",
    "format": "%.3g",
    "type": "number",
    "units": "eV",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "Egap_fit=3.5",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1021/co200012w",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?Egap_fit"
  },
  "Egap_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the electronic band gap type.",
    "title": "electronic band gap type",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "Egap_type=insulator_direct",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?Egap_type"
  },
  "energy_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the total ab initio energy per atom.",
    "title": "total energy atom",
    "format": "%.3f",
    "type": "number",
    "units": "eV/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "energy_atom=-82.1656",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?energy_atom"
  },
  "energy_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the total ab initio energy per cell.",
    "title": "total energy cell",
    "format": "%.3f",
    "type": "number",
    "units": "eV/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "energy_cell=-82.1656",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?energy_cell"
  },
  "energy_cutoff": {
    "__comment__": [
      ""
    ],
    "description": "Return the plane-wave energy cut-off used for the calculation.",
    "title": "cutoff energy",
    "format": "%d",
    "type": "number",
    "delimiter": "",
    "units": "eV",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "energy_cutoff=384.1,384.1,384.1",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?energy_cutoff"
  },
  "enthalpy_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the enthalpy per atom.",
    "title": "enthalpy atom",
    "format": "%.3f",
    "type": "number",
    "units": "eV/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "enthalpy_atom=-82.1656",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?enthalpy_atom"
  },
  "enthalpy_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the enthalpy per cell.",
    "title": "enthalpy cell",
    "format": "%.3f",
    "type": "number",
    "units": "eV/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "enthalpy_cell=-82.1656",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?enthalpy_cell"
  },
  "enthalpy_formation_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the formation enthalpy per atom.",
    "title": "formation enthalpy atom",
    "format": "%.3f",
    "type": "number",
    "units": "eV/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "enthalpy_formation_atom=-33.1587",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?enthalpy_formation_atom"
  },
  "enthalpy_formation_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the formation enthalpy per cell.",
    "title": "formation enthalpy cell",
    "format": "%.3f",
    "type": "number",
    "units": "eV/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "enthalpy_formation_cell=-33.1587",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?enthalpy_formation_cell"
  },
  "entropic_temperature": {
    "__comment__": [
      ""
    ],
    "description": "Returns the entropic temperature.",
    "title": "entropic temperature",
    "format": "%.0f",
    "type": "number",
    "units": "K",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "entropic_temperature=1072.1",
    "references": [
      "10.1038/nmat3568",
      "10.1103/PhysRevX.3.041035"
    ],
    "status": "production",
    "syntax": "$aurl/?entropic_temperature"
  },
  "files": {
    "__comment__": [
      ""
    ],
    "description": "Returns the input and output files used in the simulation.",
    "title": "I/O files",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "conditional",
    "expression": "directive",
    "example": "files=Bi_dRh_pv.33.cif,Bi_dRh_pv.33.png,CONTCAR.relax,CONTCAR.relax1,",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?files"
  },
  "forces": {
    "__comment__": [
      ""
    ],
    "description": "Returns the forces on the atoms for the relaxed structure.",
    "title": "forces relaxed",
    "format": "%g",
    "type": "numbers",
    "delimiter": ",",
    "units": "eV/&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "forces=0,-0.023928,0.000197;0,0.023928,-0.000197;...",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?forces"
  },
  "geometry": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice parameters of the relaxed simulation cell.",
    "title": "lattice parameters relaxed",
    "format": "%.4g",
    "function": "lattice",
    "type": "numbers",
    "delimiter": " ",
    "units": "",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "geometry=18.82,18.82,18.82,32.41,32.41,32.41",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "verification": [
      "energy_cutoff",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "syntax": "$aurl/?geometry"
  },
  "geometry_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice parameters of the unrelaxed simulation cell.",
    "title": "lattice parameters unrelaxed",
    "format": "%.4g",
    "function": "lattice",
    "type": "numbers",
    "delimiter": " ",
    "units": "",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "geometry_orig=18.82,18.82,18.82,32.41,32.41,32.41",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?geometry_orig"
  },
  "icsd_number": {
    "__comment__": [
      ""
    ],
    "description": "Returns the ICSD entry number",
    "title": "ICSD entry",
    "format": "%d",
    "function": "icsd",
    "type": "number",
    "delimiter": "",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "icsd_number=410874",
    "references": [
      "10.1016/j.commatsci.2012.02.002"
    ],
    "status": "development",
    "syntax": "$aurl/?icsd_number"
  },
  "keywords": {
    "__comment__": [
      ""
    ],
    "description": "This includes the list of keywords available in the entry, separated by commas.",
    "title": "Title",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "directive",
    "example": "keywords=aurl,auid,loop,code,compound,prototype,nspecies,natoms,...",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?keywords"
  },
  "kpoints": {
    "__comment__": [
      ""
    ],
    "description": "Set of k-point meshes uniquely identifying the various steps of the calculations, e.g. relaxation, static and electronic band structure (specifying the k-space symmetry points of the structure).",
    "title": "k-point mesh",
    "format": "%s",
    "type": "strings",
    "delimiter": ":,",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "kpoints=10,10,10;16,16,16;G-X-W-K-G-L-U-W-L-K+U-X",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?kpoints"
  },
  "kpoints_bands_nkpts": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of points, between the high-symmetry k-points, used for the band structure calculation.",
    "title": "k-points number",
    "format": "%d",
    "type": "number",
    "delimiter": "",
    "units": "",
    "inclusion": "optional",
    "example": "kpoints_bands_nkpts=20",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?kpoints_bands_nkpts"
  },
  "kpoints_bands_path": {
    "__comment__": [
      ""
    ],
    "description": "Returns the high-symmetry k-point path used for the band structure calculation.",
    "title": "k-points path",
    "format": "%s",
    "function": "params",
    "type": "strings",
    "delimiter": " | ",
    "units": "",
    "inclusion": "optional",
    "example": "kpoints_bands_path=G-X-W-K-G-L-U-W-K-K-U-X",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?kpoints_bands_path"
  },
  "kpoints_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the k-point grid used for the structural relaxation calculation.",
    "title": "k-points grid relax",
    "format": "%d",
    "type": "numbers",
    "delimiter": " &times; ",
    "units": "",
    "inclusion": "mandatory",
    "example": "kpoints_relax=14,14,14",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?kpoints_relax"
  },
  "kpoints_static": {
    "__comment__": [
      ""
    ],
    "description": "Returns the k-point grid used for the static calculation.",
    "title": "k-points grid static",
    "format": "%d",
    "type": "numbers",
    "delimiter": " &times; ",
    "units": "",
    "inclusion": "optional",
    "example": "kpoints_static=14,14,14",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?kpoints_static"
  },
  "lattice_system_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice system for the unrelaxed structure.",
    "title": "lattice system unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "lattice_system_orig=rhombohedral",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?lattice_system_orig"
  },
  "lattice_system_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice system for the relaxed structure.",
    "title": "lattice system relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "lattice_system_relax=rhombohedral",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?lattice_system_relax"
  },
  "lattice_variation_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice variation for the unrelaxed structure.",
    "title": "lattice variation unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "lattice_variation_orig=rhombohedral",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?lattice_variation_orig"
  },
  "lattice_variation_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the lattice variation for the relaxed structure.",
    "title": "lattice variation relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "lattice_variation_relax=rhombohedral",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?lattice_variation_relax"
  },
  "ldau_j": {
    "__comment__": [
      ""
    ],
    "description": "Returns the J parameters of the DFT+U calculation.",
    "title": "LDAU J",
    "format": "%g",
    "function": "species_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "eV",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "ldau_j=0,0,0",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?ldau_j"
  },
  "ldau_l": {
    "__comment__": [
      ""
    ],
    "description": "Returns The orbitals of the DFT+U calculation.",
    "title": "LDAU l",
    "format": "%d",
    "function": "species_resolved",
    "type": "numbers",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "ldau_l=2,0,0",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?ldau_l"
  },
  "ldau_TLUJ": {
    "__comment__": [
      ""
    ],
    "description": "This vector of numbers contains the parameters of the DFT+U calculations, based on a corrective functional inspired by the Hubbard model.",
    "title": "on site coulomb interaction",
    "format": "%s",
    "type": "numbers",
    "delimiter": ";,",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "ldau_TLUJ=2;2,0,0;5,0,0;0,0,0",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?ldau_TLUJ"
  },
  "ldau_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the type of DFT+U calculation performed.",
    "title": "LDAU type",
    "format": "%s",
    "function": "ldau",
    "type": "number",
    "delimiter": "",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "ldau_type=2",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?ldau_type"
  },
  "ldau_u": {
    "__comment__": [
      ""
    ],
    "description": "Returns the U parameters of the DFT+U calculation.",
    "title": "LDAU U",
    "format": "%g",
    "function": "species_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "eV",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "ldau_u=5,0,0",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?ldau_u"
  },
  "loop": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about the type of post-processing that was performed.",
    "title": "post-process type",
    "format": "%s",
    "type": "strings",
    "delimiter": ", ",
    "inclusion": "optional",
    "expression": "directive",
    "example": "loop=thermodynamics,bands,magnetic",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?loop"
  },
  "natoms": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of atoms in the simulation cell.",
    "title": "number of atoms",
    "format": "%d",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "natoms=12",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?natoms"
  },
  "nbondxx": {
    "__comment__": [
      ""
    ],
    "description": "Returns the nearest neighbors distances for the relaxed structure.",
    "title": "nearest neighbor distances relaxed",
    "format": "%g",
    "type": "numbers",
    "delimiter": ",",
    "units": "&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "nbondxx=1.2599,1.0911,1.0911,1.7818,1.2599,1.7818",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?nbondxx"
  },
  "node_CPU_Cores": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about the number of CPUs on the node/cluster where the calculation was performed.",
    "title": "CPU number",
    "format": "%d",
    "type": "number",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "node_CPU_Cores=12",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?node_CPU_Cores"
  },
  "node_CPU_MHz": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about the speed of CPUs on the node/cluster where the calculation was performed.",
    "title": "CPU rate",
    "format": "%d",
    "type": "number",
    "units": "MHz",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "node_CPU_MHz=12",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?node_CPU_MHz"
  },
  "node_CPU_Model": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about the model of CPUs on the node/cluster where the calculation was performed.",
    "title": "CPU model",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "node_CPU_Model=12",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?node_CPU_Model"
  },
  "node_RAM_GB": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about the RAM on the node/cluster where the calculation was performed.",
    "title": "available RAM",
    "format": "%.0f",
    "type": "number",
    "units": "Gb",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "node_RAM_GB=12",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?node_RAM_GB"
  },
  "nspecies": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of unique species in the structure.",
    "title": "species count",
    "format": "%d",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "nspecies=3",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?nspecies"
  },
  "Pearson_symbol_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pearson symbol for the unrelaxed structure.",
    "title": "Pearson symbol unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "Pearson_symbol_orig=mS32",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Pearson_symbol_orig"
  },
  "Pearson_symbol_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pearson symbol for the relaxed structure.",
    "title": "Pearson symbol relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "stress_tensor"
    ],
    "example": "Pearson_symbol_relax=mS32",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Pearson_symbol_relax"
  },
  "Pearson_symbol_superlattice": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pearson symbol of the superlattice for the relaxed structure.",
    "title": "Pearson symbol superlattice relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Pearson_symbol_superlattice=cI52",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Pearson_symbol_superlattice"
  },
  "Pearson_symbol_superlattice_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pearson symbol of the superlattice for the unrelaxed structure.",
    "title": "Pearson symbol superlattice unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Pearson_symbol_superlattice_orig=cI52",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Pearson_symbol_superlattice_orig"
  },
  "point_group_Hermann_Mauguin": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group, in Hermann-Mauguin notation, for the relaxed structure.",
    "title": "point group (Hermann-Mauguin) relaxed",
    "format": "%s",
    "function": "crystallography",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_Hermann_Mauguin=-43m",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_Hermann_Mauguin"
  },
  "point_group_Hermann_Mauguin_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group, in Hermann-Mauguin notation, for the unrelaxed structure.",
    "title": "point group (Hermann-Mauguin) unrelaxed",
    "format": "%s",
    "function": "crystallography",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_Hermann_Mauguin_orig=-43m",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_Hermann_Mauguin_orig"
  },
  "point_group_orbifold": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group orbifold for the relaxed structure.",
    "title": "point group orbifold relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_orbifold=*332",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_orbifold"
  },
  "point_group_orbifold_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group orbifold for the unrelaxed structure.",
    "title": "point group orbifold unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_orbifold_orig=*332",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_orbifold_orig"
  },
  "point_group_order": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group order for the relaxed structure.",
    "title": "point group order relaxed",
    "format": "%d",
    "type": "number",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_order=24",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_order"
  },
  "point_group_order_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group order for the unrelaxed structure.",
    "title": "point group order unrelaxed",
    "format": "%d",
    "type": "number",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_order_orig=24",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_order_orig"
  },
  "point_group_Schoenflies": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group, in Schoenflies notation, for the relaxed structure.",
    "title": "point group (Schoenflies) relaxed",
    "format": "%s",
    "function": "crystallography",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_Schoenflies=T_d",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_Schoenflies"
  },
  "point_group_Schoenflies_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group, in Schoenflies notation, for the unrelaxed structure.",
    "title": "point group (Schoenflies) unrelaxed",
    "format": "%s",
    "function": "crystallography",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "example": "point_group_Schoenflies_orig=T_d",
    "status": "production",
    "syntax": "$aurl/?point_group_Schoenflies_orig"
  },
  "point_group_structure": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group structure for the relaxed structure.",
    "title": "point group structure relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_structure=symmetric",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_structure"
  },
  "point_group_structure_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group structure for the unrelaxed structure",
    "title": "point group structure unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_structure_orig=symmetric",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_structure_orig"
  },
  "point_group_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group type for the relaxed structure.",
    "title": "point group type relaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_type=non-centrosymmetric,non-enantiomorphic,non-polar",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_type"
  },
  "point_group_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the point group type for the unrelaxed structure.",
    "title": "point group type unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "point_group_type_orig=non-centrosymmetric,non-enantiomorphic,non-polar",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?point_group_type_orig"
  },
  "positions_cartesian": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Cartesian coordinates of the atoms for the relaxed structure.",
    "title": "Cartesian coordinates relaxed",
    "format": "%.3g",
    "function": "atom_resolved",
    "type": "numbers",
    "delimiter": ";,",
    "units": "&Aring;",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "positions_cartesian=0,0,0;18.18438,0,2.85027;...",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?positions_cartesian"
  },
  "positions_fractional": {
    "__comment__": [
      ""
    ],
    "description": "Returns the fractional coordinates of the atoms for the relaxed structure.",
    "title": "fractional coordinates relaxed",
    "format": "%.3g",
    "function": "atom_resolved",
    "type": "numbers",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "positions_fractional=0,0,0;0.25,0.25,0.25;...",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?positions_fractional"
  },
  "pressure": {
    "__comment__": [
      ""
    ],
    "description": "Returns the hydrostatic pressure on the simulation cell for the unrelaxed structure.",
    "title": "pressure hydrostatic unrelaxed",
    "format": "%.3g",
    "type": "number",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "pressure=10.0",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?pressure"
  },
  "pressure_final": {
    "__comment__": [
      ""
    ],
    "description": "Returns the hydrostatic pressure on the simulation cell for the relaxed structure.",
    "title": "pressure hydrostatic relaxed",
    "format": "%.3g",
    "type": "number",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "derivative",
    "example": "pressure_final=10.0",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?pressure_final"
  },
  "pressure_residual": {
    "__comment__": [
      ""
    ],
    "description": "Returns the hydrostatic pressure, corrected by the Pulay stress, on the simulation cell for the relaxed structure.",
    "title": "pressure residual relaxed",
    "format": "%.3g",
    "type": "number",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "derivative",
    "example": "pressure_residual=10.0",
    "references": [
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?pressure_residual"
  },
  "Pullay_stress": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pulay stress correction for the calculation.",
    "title": "stress pulay",
    "format": "%g",
    "type": "number",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "derivative",
    "example": "Pullay_stress=10.0",
    "references": [
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "deprecated",
    "syntax": "$aurl/?Pullay_stress"
  },
  "Pulay_stress": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Pulay stress correcton for the calculation.",
    "title": "stress pulay",
    "format": "%g",
    "type": "number",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "derivative",
    "example": "pulay_stress=10.0",
    "references": [
      "10.1016/j.commatsci.2015.07.019",
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?pulay_stress"
  },
  "PV_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the pressure multiplied by volume per atom for the relaxed structure.",
    "title": "PV atom",
    "format": "%g",
    "type": "number",
    "units": "eV/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "PV_atom=12.13",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?PV_atom"
  },
  "PV_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the pressure multiplied by volume per atom for the relaxed structure.",
    "title": "PV cell",
    "format": "%g",
    "type": "number",
    "units": "eV/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "PV_cell=12.13",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?PV_cell"
  },
  "prototype": {
    "__comment__": [
      ""
    ],
    "description": "Returns the AFLOW prototype for the unrelaxed structure.",
    "title": "AFLOW prototype unrelaxed",
    "format": "%s",
    "type": "string",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "prototype=T0001.A2BC",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?prototype"
  },
  "reciprocal_geometry": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice parameters of the relaxed simulation cell.",
    "title": "reciprocal lattice parameters relaxed",
    "format": "%.4g",
    "function": "lattice",
    "type": "numbers",
    "uints": "&Aring;<sup>-1</sup>",
    "delimiter": " ",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_geometry_relax=0.8747,0.9747,0.8747,60.0,60.0,60.0",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_geometry"
  },
  "reciprocal_geometry_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice parameters of the unrelaxed simulation cell.",
    "title": "reciprocal lattice parameters unrelaxed",
    "format": "%.4g",
    "function": "lattice",
    "type": "numbers",
    "uints": "&Aring;<sup>-1</sup>",
    "delimiter": " ",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_geometry_orig=0.8747,0.9747,0.8747,60.0,60.0,60.0",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_geometry_orig"
  },
  "reciprocal_lattice_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice centering type for the relaxed structure.",
    "title": "reciprocal lattice centering type relaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_lattice_type=FCC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_lattice_type"
  },
  "reciprocal_lattice_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice centering type for the unrelaxed structure.",
    "title": "reciprocal lattice centering type unrelaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_lattice_type_orig=FCC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_lattice_type_orig"
  },
  "reciprocal_lattice_variation_type": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice centering type variation for the relaxed structure.",
    "title": "reciprocal lattice centering type variation relaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_lattice_variation_type=FCC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_lattice_variation_type"
  },
  "reciprocal_lattice_variation_type_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the reciprocal lattice centering type variation for the unrelaxed structure.",
    "title": "reciprocal lattice centering type variation unrelaxed",
    "format": "%s",
    "type": "string",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_lattice_variation_type_orig=FCC",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_lattice_variation_type_orig"
  },
  "reciprocal_volume_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the volume of the reciprocal cell for the relaxed structure.",
    "title": "reciprocal cell volume relaxed",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;<sup>-3</sup>",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_volume_cell=0.4733",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_volume_cell"
  },
  "reciprocal_volume_cell_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the volume of the reciprocal cell for the unrelaxed structure.",
    "title": "reciprocal cell volume unrelaxed",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;<sup>-3</sup>",
    "delimiter": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "reciprocal_volume_cell_orig=0.4733",
    "references": [
      "10.1016/j.commatsci.2010.05.010",
      "10.1016/j.commatsci.2012.02.002",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?reciprocal_volume_cell_orig"
  },
  "scintillation_attenuation_length": {
    "__comment__": [
      ""
    ],
    "description": "Returns the scintillation attenuation length.",
    "title": "attenuation length",
    "format": "%.4g",
    "type": "number",
    "units": "cm",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "scintillation_attenuation_length=2.21895",
    "references": [
      "10.1021/co200012w",
      "10.1109/TNS.2009.2027019",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?scintillation_attenuation_length"
  },
  "sg": {
    "__comment__": [
      ""
    ],
    "description": "Returns the space groups for the structure, before the first relaxation step (unrelaxed), after the first relaxation step and after the last relaxation step (relaxed), using a loose tolerance.",
    "title": "space group change loose",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "directive",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "sg=Fm-3m#225,Fm-3m#225,Fm-3m#225",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?sg"
  },
  "sg2": {
    "__comment__": [
      ""
    ],
    "description": "Returns the space groups for the structure, before the first relaxation step (unrelaxed), after the first relaxation step and after the last relaxation step (relaxed), using a tight (default) tolerance.",
    "title": "space group change tight",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "directive",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "sg2=Fm-3m#225,Fm-3m#225,Fm-3m#225",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?sg2"
  },
  "spacegroup_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the space group for the unrelaxed structure.",
    "title": "space group unrelaxed",
    "format": "%d",
    "function": "spacegroup",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "spacegroup_orig=225",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?spacegroup_orig"
  },
  "spacegroup_relax": {
    "__comment__": [
      ""
    ],
    "description": "Returns the space group number for the relaxed structure.",
    "title": "space group relaxed",
    "format": "%d",
    "function": "spacegroup",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "stress_tensor"
    ],
    "example": "spacegroup_relax=225",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014",
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?spacegroup_relax"
  },
  "species": {
    "__comment__": [
      ""
    ],
    "description": "Returns the unique species.",
    "title": "species atom",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "species=Y,Zn,Zr",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?species"
  },
  "species_pp": {
    "__comment__": [
      ""
    ],
    "description": "Returns the pseudopotential of the species.",
    "title": "species pseudopotential",
    "format": "%s",
    "function": "species_resolved",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "species_pp=Y,Zn,Zr",
    "references": [
      "10.1016/j.commatsci.2014.05.014",
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?species_pp"
  },
  "species_pp_ZVAL": {
    "__comment__": [
      ""
    ],
    "description": "Returns the number of valence electrons of the species.",
    "title": "species valence electrons",
    "title_short": "valence electrons",
    "format": "%d",
    "function": "species_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "e<sup>-</sup>",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "species_pp_ZVAL=3",
    "references": [
      "10.1016/j.commatsci.2015.07.019"
    ],
    "status": "production",
    "syntax": "$aurl/?species_pp_ZVAL"
  },
  "species_pp_version": {
    "__comment__": [
      ""
    ],
    "description": "Returns the pseudopotential version of the species.",
    "title": "species pseudopotential version",
    "format": "%s",
    "function": "species_resolved",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "declarative",
    "example": "species_pp_version=Y,Zn,Zr",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?species_pp_version"
  },
  "spinD": {
    "__comment__": [
      ""
    ],
    "description": "Returns the magnetic moment on each atom.",
    "title": "atom magnetic moment",
    "format": "%#.3g",
    "function": "atom_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "&mu;<sub>B</sub>",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "spinD=0.236,0.236,-0.023,1.005",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?spinD"
  },
  "spinD_magmom_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the magnetic moment on each atom of the unrelaxed structure.",
    "title": "atom magnetic moment unrelaxed",
    "format": "%#.3g",
    "function": "atom_resolved",
    "type": "numbers",
    "delimiter": ",",
    "units": "&mu;<sub>B</sub>",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "spinD=0.236,0.236,-0.023,1.005",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?spinD"
  },
  "spinF": {
    "__comment__": [
      ""
    ],
    "description": "Returns the magnetization of the simulation cell, at the Fermi energy.",
    "title": "magnetization cell (Fermi energy)",
    "format": "%#.3g",
    "type": "number",
    "units": "&mu;<sub>B</sub>/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "spinF=0.410879",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?spinF"
  },
  "spin_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the magnetization of the simulation cell per atom",
    "title": "magnetization atom",
    "format": "%#.3g",
    "type": "number",
    "units": "&mu;<sub>B</sub>/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "spin_atom=2.16419",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?spin_atom"
  },
  "spin_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the magnetization of the simulation cell.",
    "title": "magnetization cell",
    "format": "%#.3g",
    "type": "number",
    "units": "&mu;<sub>B</sub>/cell",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "spin_cell=2.16419",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?spin_cell"
  },
  "sponsor": {
    "__comment__": [
      ""
    ],
    "description": "Returns information about funding agencies and other sponsors for the entry.",
    "title": "sponsor",
    "format": "%s",
    "type": "strings",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "sponsor=DOD_N000141310635,NIST_70NANB12H163",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "development",
    "syntax": "$aurl/?sponsor"
  },
  "stoich": {
    "__comment__": [
      ""
    ],
    "description": "Similar to composition, returns a comma delimited stoichiometry description of the structure entry in the calculated cell.",
    "title": "unit cell stoichiometry",
    "format": "%s",
    "type": "numbers",
    "delimiter": ",",
    "inclusion": "optional",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "stoichiometry=0.5,0.25,0.25",
    "status": "deprecated",
    "syntax": "$aurl/?stoichiometry"
  },
  "stoichiometry": {
    "__comment__": [
      ""
    ],
    "description": "Returns the normalized composition of the structure.",
    "title": "stoichiometry",
    "format": "%.3g",
    "type": "numbers",
    "delimiter": ",",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "stoichiometry=0.5,0.25,0.25",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?stoichiometry"
  },
  "stress_tensor": {
    "__comment__": [
      ""
    ],
    "description": "Returns the stress tensor for the relaxed structure.",
    "title": "stress tensor",
    "format": "%g",
    "type": "numbers",
    "units": "kbar",
    "inclusion": "mandatory",
    "expression": "derivative",
    "example": "stress_tensor=-0.96,-0,-0,-0,-0.96,-0,-0,-0,-0.96",
    "references": [
      "10.1016/j.commatsci.2017.04.036"
    ],
    "status": "production",
    "syntax": "$aurl/?stress_tensor"
  },
  "valence_cell_iupac": {
    "__comment__": [
      ""
    ],
    "description": "Returns the sum of the valence electrons, based on IUPAC standards, of the atoms in the simulation cell.",
    "title": "valence electrons cell (IUPAC)",
    "format": "%d",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "valence_cell_iupac=22",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?valence_cell_iupac"
  },
  "valence_cell_std": {
    "__comment__": [
      ""
    ],
    "description": "Returns the sum of the valence electrons, based on the outermost shell(s), of the atoms in the simulation cell.",
    "title": "valence electrons cell (standard)",
    "format": "%d",
    "type": "number",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "kpoints"
    ],
    "example": "valence_cell_std=22",
    "references": [
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?valence_cell_std"
  },
  "volume_atom": {
    "__comment__": [
      ""
    ],
    "description": "Returns the volume per atom of the simulation cell for the relaxed structure.",
    "title": "volume atom relaxed",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;<sup>3</sup>/atom",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "volume_atom=100.984",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?volume_atom"
  },
  "volume_cell": {
    "__comment__": [
      ""
    ],
    "description": "Returns the volume of the simulation cell for the relaxed structure.",
    "title": "cell volume relaxed",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;<sup>3</sup>",
    "inclusion": "mandatory",
    "expression": "derivative",
    "verification": [
      "energy_cutoff",
      "forces",
      "kpoints",
      "pressure_residual",
      "stress_tensor"
    ],
    "example": "volume_cell=100.984",
    "references": [
      "10.1016/j.commatsci.2012.02.002",
      "10.1016/j.commatsci.2014.05.014"
    ],
    "status": "production",
    "syntax": "$aurl/?volume_cell"
  },
  "Wyckoff_letters": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff letters of each site for the relaxed structure.",
    "title": "Wyckoff letters relaxed",
    "title_short": "letter",
    "format": "%s",
    "function": [
      "params",
      "atom_resolved"
    ],
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_letters=g,c,a",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_letters"
  },
  "Wyckoff_letters_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff letters of each site for the unrelaxed structure.",
    "title": "Wyckoff letters unrelaxed",
    "title_short": "letter",
    "format": "%s",
    "function": [
      "params",
      "atom_resolved"
    ],
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_letters_orig=g,c,a",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_letters_orig"
  },
  "Wyckoff_multiplicities": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff multiplicity of each site for the relaxed structure.",
    "title": "Wyckoff multiplicities relaxed",
    "title_short": "multiplicity",
    "format": "%d",
    "function": "atom_resolved",
    "type": "numbers",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_multiplicities=24,8,2",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_multiplicities"
  },
  "Wyckoff_multiplicities_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff multiplicity of each site for the unrelaxed structure.",
    "title": "Wyckoff multiplicities unrelaxed",
    "title_short": "multiplicity",
    "format": "%d",
    "function": "atom_resolved",
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_multiplicities_orig=24,8,2",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_multiplicities_orig"
  },
  "Wyckoff_positions": {
    "__comment__": [
      ""
    ],
    "description": "The Wyckoff positions of each site for the relaxed structure.",
    "title": "Wyckoff positions relaxed",
    "format": "%s",
    "function": "atom_resolved",
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "correlated",
    "status": "production",
    "correlated": [
      "Wyckoff_letters",
      "Wyckoff_multiplicities",
      "Wyckoff_site_symmetries"
    ]
  },
  "Wyckoff_positions_orig": {
    "__comment__": [
      ""
    ],
    "description": "The Wyckoff positions of each site for the unrelaxed structure.",
    "title": "Wyckoff positions unrelaxed",
    "format": "%s",
    "function": "atom_resolved",
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "correlated",
    "status": "production",
    "correlated": [
      "Wyckoff_letters_orig",
      "Wyckoff_multiplicities_orig",
      "Wyckoff_site_symmetries_orig"
    ]
  },
  "Wyckoff_site_symmetries": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff symmetry of each site for the relaxed structure.",
    "title": "Wyckoff site symmetry relaxed",
    "title_short": "site symmetry",
    "format": "%s",
    "function": [
      "crystallography",
      "atom_resolved"
    ],
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_site_symmetries=m..,.-3.,m-3.",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_site_symmetries"
  },
  "Wyckoff_site_symmetries_orig": {
    "__comment__": [
      ""
    ],
    "description": "Returns the Wyckoff symmetry of each site for the unrelaxed structure.",
    "title": "Wyckoff site symmetry unrelaxed",
    "title_short": "site symmetry",
    "format": "%s",
    "function": [
      "crystallography",
      "atom_resolved"
    ],
    "type": "strings",
    "units": "",
    "inclusion": "optional",
    "expression": "declarative",
    "example": "Wyckoff_site_symmetries_orig=m..,.-3.,m-3.",
    "references": [
      "10.1107/S2053273318003066"
    ],
    "status": "production",
    "syntax": "$aurl/?Wyckoff_site_symmetries_orig"
  },
  "lat_distortion_misfit_mean_0K": {
    "description": "Mean lattice distortion misfit at 0K.",
    "title": "lattice distortion misfit mean 0K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_misfit_mean_0K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_misfit_stdev_0K": {
    "description": "Standard deviation of lattice distortion misfit at 0K.",
    "title": "lattice distortion misfit stdev 0K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_misfit_stdev_0K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_relax_distance_mean_0K": {
    "description": "Mean lattice distortion relaxation distance at 0K.",
    "title": "lattice distortion relax distance mean 0K",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_relax_distance_mean_0K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_relax_distance_stdev_0K": {
    "description": "Standard deviation of lattice distortion relaxation distance at 0K.",
    "title": "lattice distortion relax distance stdev 0K",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_relax_distance_stdev_0K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "solubility_parameter_0K": {
    "description": "Solubility parameter at 0K.",
    "title": "solubility parameter 0K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?solubility_parameter_0K"
  },
  "lat_distortion_misfit_mean_300K": {
    "description": "Mean lattice distortion misfit at 300K.",
    "title": "lattice distortion misfit mean 300K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_misfit_mean_300K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_misfit_stdev_300K": {
    "description": "Standard deviation of lattice misfit at 300K.",
    "title": "lattice distortion misfit stdev 300K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_misfit_stdev_300K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_relax_distance_mean_300K": {
    "description": "Mean lattice distortion relaxation distance at 300K.",
    "title": "lattice distortion relax distance mean 300K",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_relax_distance_mean_300K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "lat_distortion_relax_distance_stdev_300K": {
    "description": "Standard deviation of lattice distortion relaxation distance at 300K.",
    "title": "lattice distortion relax distance stdev 300K",
    "format": "%.4g",
    "type": "number",
    "units": "&Aring;",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?lat_distortion_relax_distance_stdev_300K",
    "references": [
      "10.1038/s41524-020-00483-4"
    ]
  },
  "solubility_parameter_300K": {
    "description": "Solubility parameter at 300K.",
    "title": "solubility parameter 300K",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?solubility_parameter_300K",
    "references": [
      "10.1002/adma.201702712"
    ]
  },
  "species_sublattice": {
    "description": "Species present on each sublattice.",
    "title": "species sublattice",
    "format": "%s",
    "type": "strings",
    "inclusion": "optional",
    "expression": "declarative",
    "status": "production",
    "syntax": "$aurl/?species_sublattice"
  },
  "delta_electronegativity_Pearson_sublattice": {
    "description": "Mismatch in Pearson electronegativity for each sublattice.",
    "title": "delta electronegativity Pearson sublattice",
    "format": "%.4g",
    "type": "numbers",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?delta_electronegativity_Pearson_sublattice",
    "references": [
      "10.1016/S0022-3093(03)00155-8",
      "10.1016/S1002-0071(12)60080-X"
    ]
  },
  "delta_radius_covalent_PT_sublattice": {
    "description": "Mismatch in covalent radius (periodictable.com) for each sublattice.",
    "title": "delta radius covalent PT sublattice",
    "format": "%.4g",
    "type": "numbers",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?delta_radius_covalent_PT_sublattice",
    "references": [
      "10.1107/S0567739476001551",
      "10.1002/adem.200700240"
    ]
  },
  "param_geo_radius_covalent_PT_atom": {
    "description": "Geometric parameter of covalent radius (periodictable.com) per atom.",
    "title": "Geometric parameter radius covalent PT atom",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?param_geo_radius_covalent_PT_atom",
    "references": [
      "10.1016/j.intermet.2014.04.019"
    ]
  },
  "S_config_atom": {
    "description": "Configurational entropy per atom.",
    "title": "S config atom",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?S_config_atom",
    "references": [
      "10.1021/acsnano.4c03435",
      "10.1016/j.cossms.2017.09.002"
    ]
  },
  "S_config_partial_atom": {
    "description": "Configurational entropy per atom, distinguishing only between metal and non-metal sites.",
    "title": "S config partial atom",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?S_config_partial_atom"
  },
  "VEC": {
    "description": "Valence Electron Concentration (valence electrons per atom).",
    "title": "valence electron concentration",
    "title_short": "VEC",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?VEC",
    "references": [
      "10.1002/adma.202102904",
      "10.1016/j.actamat.2024.120117"
    ]
  },
  "Hmix_miedema": {
    "description": "Mixing enthalpy calculated using Miedema's model.",
    "title": "mixing enthalpy Miedema",
    "format": "%.4g",
    "type": "number",
    "inclusion": "optional",
    "expression": "derivative",
    "status": "production",
    "syntax": "$aurl/?Hmix_miedema",
    "references": [
      "https://www.osti.gov/biblio/4382858"
    ]
  }
}
