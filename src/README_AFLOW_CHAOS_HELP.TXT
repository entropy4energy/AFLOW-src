-PRE START-
*******************************************************************************
                 ██████╗ ██╗  ██╗ █████╗  █████╗  ███████╗
                 ██╔═══╝ ██║  ██║██╔══██╗██╔══██╗██╔═════╝
                 ██║     ███████║███████║██║  ██║███████╗
                 ██║     ██╔══██║██╔══██║██║  ██║╚════██║
                 ╚██████╗██║  ██║██║  ██║╚█████╔╝███████║
                  ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚════╝ ╚══════╝

                        WELCOME TO CHAOS database
    Combinatorial High-throughput Analysis & Optimization for Synthesis
             COREY OSES Johns Hopkins University 2022 - 2025
*******************************************************************************
LATEST VERSION OF THE FILE: v1.0                             
*******************************************************************************
-PRE END-

-GENERAL START-
Welcome to **CHAOS** — the Combinatorial High-throughput Analysis &
Optimization for Synthesis database, a comprehensive open-access platform
designed to advance next-generation materials discovery.

Spanning metals, intermetallics, ceramics, semiconductors, catalysts, and
energy-relevant compounds, CHAOS provides rigorously validated thermodynamic,
structural, electronic, mechanical, and functional properties with
standardized and fully machine-actionable data.


*************** CHAOS search API Instruction ***************

**Base URL**
https://s4e.ai/API/chaos/?

All queries (called <summons>) are written directly after the ‘?’
No extra path, no slashes
Example: https://s4e.ai/API/chaos/?<summons>

**Quick examples**
- This help page:                     https://s4e.ai/API/chaos/?help
- Schema:                             https://s4e.ai/API/chaos/?schema
- Cell potential ≥ -25 eV/cell:       https://s4e.ai/API/chaos/?energy_cell(-25*)

### Query Structure
Every query consists of two logical parts:

| Part       | Purpose                                                        |
|------------|----------------------------------------------------------------|
| Matchbook  | What to search for & which properties to return/filter         |
| Directives | How the output is formatted and delivered                      |

### Quick Access
| What you want                    | Query example          |
|----------------------------------|------------------------|
| This help page                   | help                   |
| Full list of all properties      | schema                 |
| Details on one property          | help(energy_cell)      |
| All aliases                      | help(aliases)          |
| All directives                   | help(directives)       |

### Logic Operators Cheat Sheet
| Symbol | Meaning                          | Example                              |
|--------|----------------------------------|--------------------------------------|
| N*     | ≥ N                              | energy_cell(-20*)                    |
| *N     | ≤ N                              | energy_cell(*-20)                    |
| ,      | AND                              | density(3*),species(O)               |
| :      | OR                               | density((1*,*2):(3*,*4))             |
| !      | NOT                              | species(!H)                          |
| $      | Suppress default fields          | $compound,$Pearson_symbol_relax      |
| @      | Tensor / species-specific filter | ael_compliance_tensor(@(3,3)(0*))    |


### Chaos API – Query Syntax & Directives
### 1. Basic Numerical Queries
| Goal                                           | Query example                            |
|------------------------------------------------|------------------------------------------|
| Entries that have cell potential               | energy_cell(*)                           |
| Cell potential ≥ -40.0 and ≤ -20.01 eV/cell    | energy_cell(1*,*1.01)                    |
| Multiple OR range                              | density((1*,*1.01):(2*,*2.01)),paging(2) |
| Second page (64 entries per page)              | density(3*),paging(2)                    |

### 2. Pagination
| Goal                              | Query example     |
|-----------------------------------|-------------------|
| Page 3 (default size 64)          | paging(3)         |
| All results (no pagination)       | paging(0)         |
| Page J with custom size K         | paging(J,K)       |

**Important note** `paging(0)` / `paging(-0)`**  returns all possible results.
Large data responses can crash your browser.  
Prefer using `wget`, `curl`, or any other direct download tool for full retrieval.

### 3. String Matching
| Goal                          | Query example                      |
|-------------------------------|------------------------------------|
| Contains “28”                 | Pearson_symbol_relax(*'28'*)       |
| Does NOT contain “F”          | Pearson_symbol_relax(!*'F'*)       |

Browser percent-encoding is handled automatically.
Don't worry about browser percent-encoding, CHAOS API will still handle automatically.

### 4. Multiple Properties & Suppression
| Goal                                      | Query example                                     |
|-------------------------------------------|---------------------------------------------------|
| Density + Pearson symbol (all results)    | density(6*,*7.51),Pearson_symbol_relax,paging(-0) |
| Same but hide compound name               | density(1*,*1.01),Pearson_symbol_relax,$compound  |

### 5. Inversion
| Goal                                      | Query example                         |
|-------------------------------------------|---------------------------------------|
| No “F” in Pearson symbol (all results)    | Pearson_symbol_relax(!*'F'*),paging(0)|
| Cell potential strictly > -10 eV/cell     | energy_cell(!*-10)                    |

### 6. Tensors & Species-Specific Filters
| Goal                                              | Query example                           |
|---------------------------------------------------|-----------------------------------------|
| Stress tensor: first diagonal element ≥ 0         | stress_tensor(@(0)(0*))                 |
| Elastic compliance tensor: element (1,1) ≤ 0      | ael_compliance_tensor(@(1,1)(0*))       |
| Any chlorine atom has net charge ≤ 0              | bader_net_charges(@Cl(*0))              |
| Any iron atom positively charged OR any oxygen ≤ 0| bader_net_charges(@Fe(0*):@O(*0))       |

### 7. Popular Aliases
| Goal                  | Query example             |
|-----------------------|---------------------------|
| Any alkali metal      | species(AlkaliMetals)     |
| Any transition metal  | species(TransitionMetals) |
| Any lanthanide        | species(Lanthanides)      |
| Any halogen           | species(Halogens)         |
| Any chalcogen         | species(Chalcogens)       |

### 8. Popular Directives
| Directive | Effect                                |
|-----------|---------------------------------------|
| format    | Output format: json or html           |
| paging    | Controls pagination                   |
| catalog   | Filters by database subset            |
-GENERAL END-

-ALIASES START-
Welcome to CHAOS search API (version 1.0)
CHAOS aliases: https://s4e.ai/API/chaos/?help(aliases)
The available aliases are listed together to make it quick 
and easy to see all available aliases. If any of the following aliases 
occur in a summons it will be replaced with the associated string expansion.
{
    "alias": "AlkaliMetals",
    "expansion": "(Li:Na:K:Rb:Cs)"
},
{
    "alias": "TransitionMetals",
    "expansion": "(Sc:Ti:V:Cr:Mn:Fe:Co:Ni:Cu:Zn:Y:Zr:Nb:Mo:Tc:Ru:Rh:Pd:Ag:Cd:Hf:Ta:W:Re:Os:Ir:Pt:Au:Hg)"
},
{
    "alias": "OtherMetals",
    "expansion": "(Al:Ga:Ge:In:Sn:Sb:Tl:Pb:Bi)"
},
{
    "alias": "NonMetals",
    "expansion": "(He:B:C:N:O:F:Ne:Si:P:S:Cl:Ar:As:Se:Br:Kr:Te:I:Xe)"
},
{
    "alias": "Metals",
    "expansion": "(Li:Be:Na:Mg:Al:K:Ca:Sc:Ti:V:Cr:Mn:Fe:Co:Ni:Cu:Zn:Ga:Rb:Sr:Y:Zr:Nb:Mo:Tc:Ru:Rh:Pd:Ag:Cd:In:Sn:Sb:Cs:Ba:La:Ce:Pr:Nd:Pm:Sm:Eu:Gd:Tb:Dy:Ho:Er:Tm:Yb:Lu:Hf:Ta:W:Re:Os:Ir:Pt:Au:Hg:Tl:Pb:Bi)"
},
{
    "alias": "AlkaliEarths",
    "expansion": "(Be:Mg:Ca:Sr:Ba)"
},
{
    "alias": "Lanthanides",
    "expansion": "(La:Ce:Pr:Nd:Pm:Sm:Eu:Gd:Tb:Dy:Ho:Er:Tm:Yb:Lu)"
},
{
    "alias": "BoronGroup",
    "expansion": "(B:Al:Ga:In:Tl)"
},
{
    "alias": "CarbonGroup",
    "expansion": "(C:Si:Ge:Sn:Pb)"
},
{
    "alias": "Pnictogens",
    "expansion": "(N:P:As:Sb:Bi)"
},
{
    "alias": "Chalcogens",
    "expansion": "(O:S:Se:Te:Po)"
},
{
    "alias": "Halogens",
    "expansion": "(F:Cl:Br:I:At)"
},
{
    "alias": "aflow_prototype_parameters_orig",
    "expansion": "(aflow_prototype_params_list_orig,aflow_prototype_params_values_orig)"
},
{
    "alias": "aflow_prototype_parameters_relax",
    "expansion": "(aflow_prototype_params_list_relax,aflow_prototype_params_values_relax)"
},
{
    "alias": "Wyckoff_positions_orig",
    "expansion": "(Wyckoff_letters_orig,Wyckoff_multiplicities_orig,Wyckoff_site_symmetries_orig)"
},
{
    "alias": "Wyckoff_positions",
    "expansion": "(Wyckoff_letters,Wyckoff_multiplicities,Wyckoff_site_symmetries)"
}
-ALIASES END-

-DIRECTIVES START-
Welcome to Chaos - The S4E search API (version 1.0)
Chaos directives: https://s4e.ai/API/chaos/?help(directives)

This is the help file for the Chaos directives in version 1.0
Directives on format:
    Output format: json (structured data, default) or html (table view in browser).
        <Summons> = format(json)
        
Directives on paging:
    Controls results per page. Use 0 for all results (no paging); 1 for first page; negative for reverse sort (e.g., -1).
        <Summons> = paging(0)
        
Directives on catalog:
    Filter by database subset (e.g., ICSD, LIB3).
        <Summons> = catalog(ICSD)       
-DIRECTIVES END-

-POST START-


*************** Contribution Guidelines ***************

To contribute data or expand the database, generate entries using aflow++
(open-source at https://github.com/entropy4energy/AFLOW-src) and submit 
via S4E Laboratory (contact corey.oses@jhu.edu). 
Suggestions for new descriptors or structures welcome.

Team: Jiayue Hu, Shao-Yu Tseng, Matthew Brownrigg, Guangshuai Han,
Tianhao Li, William Shiber, Xiao Xu, Maya Niyogi, Keith Clark,
Corey Oses (PI, Department of Materials Science and Engineering,
Johns Hopkins University).


*************** References & License ***************

**References**
CHAOS: Submitted (2025)
aflow.org: https://doi.org/10.1016/j.commatsci.2022.111808
AFLUX: https://doi.org/10.1016/j.commatsci.2017.04.036
AFLOW REST API: https://doi.org/10.1016/j.commatsci.2014.05.014

**License**
CHAOS is made available under the Open Database License: 
AFLOW-src/src/README_AFLOW_CHAOS_ODBL_LICENSE.TXT. Any rights in individual 
contents of the database are licensed under the Database Contents License: 
http://opendatacommons.org/licenses/dbcl/1.0/

**************** Enjoy the Chaos! ****************
-POST END-
 
