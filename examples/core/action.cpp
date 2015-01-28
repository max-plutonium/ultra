//! [currying]
int func(char, short, float);
...
action<int (char, short, float)> a1(func);
action<int (short, float)> a2 = a1(127);
action<int (float)> a3 = a2(32767);

int res = a3(12.34f);
res = a1(126)(32766)(12.34f);
//! [currying]
