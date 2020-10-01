# remove section numbers for subheadings
# Based on Wagner Macedo's filter.py posted at
# https://groups.google.com/forum/#!msg/pandoc-discuss/RUC-tuu_qf0/h-H3RRVt1coJ
import pandocfilters as pf

sec = 0

def do_filter(k, v, f, m):
    global sec
    if sec > 2 or (k == "Header" and v[0] < 3):
        return []
    if k == "Header" and v[0] > 2:
        sec += 1
        v[1][1].append('unnumbered')
        return pf.Header(v[0], v[1], v[2])

if __name__ == "__main__":
    pf.toJSONFilter(do_filter)
