# 1. convert tables to use 'tabu'
# Based on Wagner Macedo's filter.py posted at
# https://groups.google.com/forum/#!msg/pandoc-discuss/RUC-tuu_qf0/h-H3RRVt1coJ
import pandocfilters as pf

def latex(s):
    return pf.RawBlock('latex', s)

def inlatex(s):
    return pf.RawInline('latex', s)

def tbl_caption(s):
    return pf.Para([inlatex(r'\caption{')] + s[1][0]['c'] + [inlatex(r'}')])

def tbl_alignment(a):
    aligns = {
        "AlignDefault": 'l',
        "AlignLeft": 'l',
        "AlignCenter": 'c',
        "AlignRight": 'r',
    }
    s = ''.join(map(lambda al: aligns[al['t']], a[:-1]))
    s += 'X[1,'+aligns[a[-1]['t']]+']'
    return s

def tbl_headers(s):
    s = s[1][0][1]
    result = s[0][4][0]['c']
    for i in range(1, len(s)):
        result.append(inlatex(' & '))
        result.extend(s[i][4][0]['c'])
    result.append(inlatex(r'\\' '\n'))
    return pf.Para(result)

def tbl_contents(s):
    s = s[3]
    result = []
    for row in s:
        para = []
        for col in row[1]:
            if col[4]:
                para.extend(col[4][0]['c'])
            para.append(inlatex(' & '))
        result.extend(para)
        result[-1] = inlatex(r'\\' '\n')
    return pf.Para(result)

def do_filter(k, v, f, m):
    if k == "Table":
        a, w = zip(*v[2])
        if w[0]['t'] == "ColWidthDefault":
            w = [1 for e in w]
            wd = ''
            ha = r'\centering'
        else:
            w = [e['c'] for e in w]
            wd = '*'
            ha = r'\raggedright'
        return [latex(r'\begin{table'+wd+'}[!h]'),
                tbl_caption(v[1]),
                latex(ha),
                latex(r'\begin{tabu}{' + tbl_alignment(a) + '}'),
                latex(r'\toprule'),
                tbl_headers(v[3]),
                latex(r'\midrule'),
                tbl_contents(v[4][0]),
                latex(r'\bottomrule' '\n' r'\end{tabu}'),
                latex(r'\end{table'+wd+'}')]

if __name__ == "__main__":
    pf.toJSONFilter(do_filter)
