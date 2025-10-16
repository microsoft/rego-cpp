../../rego-cpp/build/tools/rego -l Info -1 -d policy.rego -i input.json -q data.policy.allow

rego 0.4.6 (main:5a6b476, Sat, 1 Feb 2025 14:27:17 +0000)[Clang 15.0.7] on linux
Setting input from file: "input.json"
---------
Pass	Iterations	Changes	Time (us)
parse	140723091841392	94684818245766	140723091841448
groups	2	4	52
structure	2	24	134
---------

---------
Pass	Iterations	Changes	Time (us)
from_json	140723091842448	94684818245766	140723091842504
from_json	1	28	114
---------

Adding module file: "policy.rego"
---------
Pass	Iterations	Changes	Time (us)
parse	140723091841520	94684818245766	140723091841576
prep	1	6	89
keywords	1	6	111
some_every	1	0	55
ref_args	2	13	147
refs	2	23	280
groups	1	7	148
terms	1	23	155
unary	1	0	118
arithbin_first	2	1	193
arithbin_second	1	0	150
comparison	2	3	185
membership	1	0	138
assign	2	2	201
else_not	1	1	184
collections	1	0	159
lines	2	47	344
rules	1	8	305
literals	1	17	226
structure	1	14	195
---------

Query: data.policy.allow
---------
Pass	Iterations	Changes	Time (us)
parse	140723091840816	94684818245766	140723091840872
prep	1	1	22
keywords	1	0	16
some_every	1	0	14
ref_args	2	2	27
refs	2	1	32
groups	1	0	28
terms	1	1	30
unary	1	0	28
arithbin_first	1	0	31
arithbin_second	1	0	33
comparison	1	0	31
membership	1	0	33
assign	1	0	35
else_not	1	0	37
collections	1	0	36
lines	2	2	47
rules	1	0	48
literals	1	1	50
structure	1	1	46
---------

---------
Pass	Iterations	Changes	Time (us)
unify	140723091841776	94684818245766	140723091841832
strings	1	13	211
merge_data	2	2	291
varrefheads	1	0	228
lift_refheads	2	8	565
symbols	2	40	538
replace_argvals	1	0	240
lift_query	2	4	289
expand_imports	1	1	297
constants	2	6	296
explicit_enums	1	0	211
body_locals	1	0	426
value_locals	1	0	294
compr_locals	1	0	236
rules_to_compr	1	0	219
compr	1	0	217
absolute_refs	2	7	470
merge_modules	3	4	353
datarule	1	0	239
skips	1	0	322
infix	1	5	302
assign	1	26	587
skip_refs	1	9	606
simple_refs	2	75	830
init	1	0	1204
implicit_enums	1	0	425
enum_locals	1	0	460
rulebody	2	31	791
lift_to_rule	1	0	303
functions	2	63	665
unify	1	0	2632
result	2	1	29
---------

{"expressions":[true]}

