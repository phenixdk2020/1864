import sys, ast
for l in open(sys.argv[1]):
    if l.startswith('MEASURE'): print(l.strip()); continue
    d = ast.literal_eval(l.strip())
    print(d['frame'], d['clearance'], d['min_gap_above'], d['n_contact_verts'], 'lat', d['lat_at_shoulder'], 'hand_fwd', d['hand_fwd'], 'tilt', d['tilt_back_deg'], 'muz', d['muzzle_over_head'], 'minfwd', d['min_fwd_of_shoulder'], 'butt->bone', d['butt_to_hand_bone'], 'butt->centre', d['butt_to_hand_centre'], 'below', d['butt_below_hand'])
# summary
import re
cur = None; rows = {}
for l in open(sys.argv[1]):
    if l.startswith('MEASURE'): cur = l.split()[1]; rows[cur] = []; continue
    rows[cur].append(ast.literal_eval(l.strip()))
for k, r in rows.items():
    c = [x['clearance'] for x in r]; ab = [x['min_gap_above'] for x in r if x['min_gap_above'] is not None]
    print('SUMMARY', k, 'clear min/max cm', round(min(c) * 100, 2), round(max(c) * 100, 2), 'n', len(c), 'neg frames', [x['frame'] for x in r if x['clearance'] < 0], '>2cm', [x['frame'] for x in r if x['clearance'] > 0.02],
          'above_min cm', round(min(ab) * 100, 1), 'minfwd cm', round(min(x['min_fwd_of_shoulder'] for x in r) * 100, 1), 'muz_over_head min cm', round(min(x['muzzle_over_head'] for x in r) * 100, 1), 'tilt', min(x['tilt_back_deg'] for x in r), max(x['tilt_back_deg'] for x in r), 'hand_fwd', min(x['hand_fwd'] for x in r), max(x['hand_fwd'] for x in r))
    print('BUTT', k, 'to hand bone max cm', round(max(x['butt_to_hand_bone'] for x in r) * 100, 1), 'to hand centre max cm', round(max(x['butt_to_hand_centre'] for x in r) * 100, 1), 'below-centre min/max cm', round(min(x['butt_below_hand'] for x in r) * 100, 1), round(max(x['butt_below_hand'] for x in r) * 100, 1))
    print('V8', k, 'out_deg min/max', min(x['out_deg'] for x in r), max(x['out_deg'] for x in r), 'hand->body(non-arm) min cm', round(min(x['hand_clear'] for x in r) * 100, 1), 'rifle(low)->body min cm', round(min(x['rifle_low_clear'] for x in r) * 100, 1))
