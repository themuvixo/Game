"""Assembles the 35 stages from hand-authored chunks and statically validates reachability.
Physics envelope used by the validator matches src/Player.cpp constants (conservative):
  jump rise <= 3 tiles, horizontal jump <= 4 tiles (<= 3 when rising 3), drop with <= 5 tiles drift,
  spring rise <= 7, bounce pad rise <= 9, water = free swimming, wind-up column = rise to its top + 3."""
import os, random, sys
from collections import deque
from level_chunks import C

H = 18
OUT = os.path.join(os.path.dirname(__file__), '..', 'assets', 'levels')

WORLD_MECH = {
    1: dict(A='w1_mushrooms', B='w1_pond', C='w1_canopy', D='falling', music='world1'),
    2: dict(A='w2_rhythm', B='w2_drums', C='w2_springs', D='moving', music='world2'),
    3: dict(A='w3_spinners', B='w3_door', C='w3_lift', D='vertical', music='world3'),
    4: dict(A='w4_ice', B='w4_storm', C='w4_icefall', D='w4_updraft', music='world4'),
    5: dict(A='w5_lava', B='w5_ferry', C='w5_ruins', D='falling', music='world5'),
}
NAMES = {
    1: ['Whispering Glade', 'Mushroom Hollow', 'Petalfall Falls', 'Moonlit Canopy', 'Glowroot Gauntlet', 'Hidden Starwell', "Thornmother's Bloom"],
    2: ['Drumroll Dunes', 'Harmony Heights', 'Cymbal Cliffs', 'Tempo Terrace', 'Crescendo Run', 'Secret Encore', "Bongobrute's Stage"],
    3: ['Neon Alley', 'Billboard Boulevard', 'Glitch Gardens', 'Skyline Lifts', 'Circuit Sprint', 'Pixel Vault', 'Sentinel Tower'],
    4: ['Snowdrift Path', 'Crystal Caverns', 'Blizzard Pass', 'Glacier Steps', 'Avalanche Dash', 'Aurora Grotto', "Glacius' Lair"],
    5: ['Ashen Ruins', 'Magma Falls', 'Cinder Bridge', 'Obsidian Temple', 'Inferno Climb', 'Ember Sanctum', 'Throne of Shadows'],
}
TEMPLATES = {
    1: ['start', 'tut_attack', 'flat', 'A', 'gap', 'cage_ledge', 'checkpoint', 'steps', 'B', 'enemies_mix', 'cage_ledge', 'upper_route', 'secret_room', 'exit'],
    2: ['start', 'flat', 'B', 'pillars', 'cage_ledge', 'A', 'checkpoint', 'gap_wide', 'secret_room', 'D', 'C', 'cage_ledge', 'spikes', 'enemies_mix', 'cage_ledge', 'exit'],
    3: ['start', 'A', 'steps', 'cage_ledge', 'C', 'checkpoint', 'D', 'secret_key', 'B', 'vertical', 'enemies_mix', 'cage_ledge', 'exit'],
    4: ['start', 'C', 'gap_wide', 'cage_ledge', 'breakwall', 'A', 'checkpoint', 'vertical_climb', 'moving', 'secret_room', 'B', 'spikes', 'cage_ledge', 'D', 'cage_ledge', 'exit'],
    5: ['start', 'A', 'B', 'C', 'D', 'cage_ledge', 'checkpoint', 'gap_wide', 'A', 'spikes', 'moving', 'cage_ledge', 'B', 'checkpoint', 'C', 'pillars', 'cage_ledge', 'exit'],
    6: ['start', 'upper_route', 'A', 'secret_room', 'B', 'cage_ledge', 'checkpoint', 'C', 'D', 'cage_ledge', 'A', 'gap_wide', 'secret_room', 'cage_ledge', 'exit'],
    7: ['start', 'A', 'enemies_mix', 'B', 'boss_entry', 'boss_arena'],
}
ENEMY_POOL = {1: 'aab', 2: 'abef', 3: 'abcsf', 4: 'abcefs', 5: 'abcefs'}
PAR = {1: 110, 2: 140, 3: 130, 4: 150, 5: 120, 6: 150, 7: 180}
SOLID = set('#BIdH')        # H is drawn solid but passable; handled below
STAND_ON = set('#=BIDFSU')  # cells you can stand on top of
PASS = set('.oghKEurC?XPabcefsZtw<>~R')

def chunk_rows(name):
    rows = C[name]
    w = len(rows[0])
    pad = ['.' * w] * (H - len(rows))
    return pad + rows

def assemble(world, stage):
    m = WORLD_MECH[world]
    seq = [m.get(x, x) for x in TEMPLATES[stage]]
    if world == 1 and stage == 5:
        seq.insert(-1, 'upgrade')
    grid = [''] * H
    for name in seq:
        rows = chunk_rows(name)
        for y in range(H):
            grid[y] += rows[y]
    # boundary walls
    grid = ['#' + r + '#' for r in grid]
    grid = [list(r) for r in grid]
    if stage != 7:
        # remove spare boss markers
        pass
    # key only in stage 3; cages/relic/gem stay as authored
    rng = random.Random(world * 100 + stage)
    w = len(grid[0])
    # sprinkle extra enemies on solid ground cells, deterministic
    extra = {1: 1, 2: 4, 3: 6, 4: 8, 5: 10}[world] + (stage if stage <= 5 else 2)
    if stage == 7:
        extra = world + 1
    cand = []
    for x in range(20, w - 12):
        y = H - 4
        if grid[y + 1][x] == '#' and all(grid[yy][x] == '.' for yy in range(y - 2, y + 1)):
            near = any(grid[yy][xx] in 'CXPZ' for yy in range(H) for xx in range(max(0, x - 5), min(w, x + 6)))
            if not near:
                cand.append(x)
    rng.shuffle(cand)
    placed = []
    for x in cand:
        if len(placed) >= extra:
            break
        if all(abs(x - p) > 6 for p in placed):
            t = rng.choice(ENEMY_POOL[world])
            yy = H - 4 if t != 'b' else H - 7
            grid[yy][x] = t
            placed.append(x)
    return [''.join(r) for r in grid], seq

def validate(grid, label):
    w = len(grid[0])
    g = [list(r) for r in grid]
    def cell(x, y):
        if x < 0 or x >= w:
            return '#'
        if y < 0:
            return '.'
        if y >= H:
            return 'VOID'
        return g[y][x]
    def free(x, y):
        c = cell(x, y)
        return c != 'VOID' and (c not in SOLID or c in 'BHd') and c not in '^L'
    def deadly(x, y):
        return cell(x, y) in '^L'
    # platform positions expanded for moving / vertical platforms
    extra_stand = set()
    for y in range(H):
        for x in range(w):
            c = g[y][x]
            if c == 'M':
                for dx in range(-3, 4):
                    extra_stand.add((x + dx, y))
                    extra_stand.add((x + dx + 1, y))
            if c == 'V':
                for dy in range(-3, 4):
                    extra_stand.add((x, y + dy))
                    extra_stand.add((x + 1, y + dy))
            if c == 'R':
                pass
    def can_stand(x, y):
        if not (free(x, y) and free(x, y - 1)):
            return False
        if deadly(x, y):
            return False
        below = cell(x, y + 1)
        if below in STAND_ON or (x, y + 1) in extra_stand or below == 'd':
            return True
        if below == 'H':
            return True
        if cell(x, y) == '~':
            return True  # swimming
        return False
    def bonus(x, y):
        b = cell(x, y + 1)
        if b == 'S':
            return 7
        if b == 'U':
            return 9
        return 0
    start = None
    for y in range(H):
        for x in range(w):
            if g[y][x] == 'P':
                start = (x, y)
    assert start, label + ' no start'
    seen = {start}
    dq = deque([start])
    while dq:
        x, y = dq.popleft()
        rise = 3 + bonus(x, y)
        # wind column: if standing in or next to wind, extend rise
        for yy in range(y, -1, -1):
            if cell(x, yy) == 'w':
                rise = max(rise, y - yy + 3)
        swim = cell(x, y) == '~' or cell(x, y - 1) == '~'
        for dy in range(-rise, 10):
            for dx in range(-5, 6):
                nx, ny = x + dx, y + dy
                if (nx, ny) in seen or ny < -1 or ny >= H:
                    continue
                if dy < 0:
                    maxdx = 4 if -dy <= 2 else 3
                    if bonus(x, y) or rise > 3:
                        maxdx = 4
                elif dy == 0:
                    maxdx = 4
                else:
                    maxdx = 5
                if swim and abs(dy) <= 2:
                    maxdx = max(maxdx, 2)
                if abs(dx) > maxdx:
                    continue
                if not can_stand(nx, ny):
                    continue
                # crude arc clearance: need headroom above the higher of the two cells and a free column path
                top = min(y, ny) - 1
                ok = True
                stepx = 1 if dx > 0 else -1
                for xx in range(x, nx + stepx, stepx) if dx != 0 else [x]:
                    if not free(xx, top) and cell(xx, top) not in 'BHd':
                        ok = False
                        break
                if not ok:
                    continue
                seen.add((nx, ny))
                dq.append((nx, ny))
    problems = []
    def reachable_item(x, y, reach=3):
        for (sx, sy) in seen:
            if abs(sx - x) <= reach and -1 <= sy - y <= 4 + bonus(sx, sy) and abs(sx - x) + max(0, sy - y - 1) <= 6:
                return True
        return False
    counts = {'o': 0, 'g': 0, 'E': 0, 'K': 0, 'r': 0, 'X': 0, 'Z': 0, 'C': 0}
    for y in range(H):
        for x in range(w):
            c = g[y][x]
            if c in counts:
                counts[c] += 1
                if c in 'XZgEKrC' and not reachable_item(x, y, 2 if c in 'XC' else 3):
                    problems.append('%s unreachable at %d,%d' % (c, x, y))
                if c == 'o' and not reachable_item(x, y):
                    problems.append('ting unreachable at %d,%d' % (x, y))
    return problems, counts

def main():
    os.makedirs(OUT, exist_ok=True)
    ok = True
    summary = []
    for world in range(1, 6):
        for stage in range(1, 8):
            grid, seq = assemble(world, stage)
            probs, counts = validate(grid, 'w%ds%d' % (world, stage))
            kind = 'normal' if stage <= 4 else ('challenge' if stage == 5 else ('secret' if stage == 6 else 'boss'))
            music = 'boss_final' if (stage == 7 and world == 5) else ('boss' if stage == 7 else WORLD_MECH[world]['music'])
            if probs:
                ok = False
                print('w%d-%d %s' % (world, stage, NAMES[world][stage - 1]), probs[:6], '... (%d)' % len(probs))
            with open(os.path.join(OUT, 'w%d_s%d.txt' % (world, stage)), 'w') as f:
                f.write('name=%s\nworld=%d\nstage=%d\nkind=%s\nmusic=%s\npar=%d\nchunks=%s\n---\n' %
                        (NAMES[world][stage - 1], world, stage, kind, music, PAR[stage] - (world - 1) * 4 if stage != 7 else PAR[7], ','.join(seq)))
                f.write('\n'.join(grid) + '\n')
            summary.append((world, stage, len(grid[0]), counts))
    for s in summary:
        print('w%d s%d width=%3d tings=%3d gems=%d cages=%d key=%d relic=%d checkpoints=%d exit=%d boss=%d' %
              (s[0], s[1], s[2], s[3]['o'], s[3]['g'], s[3]['E'], s[3]['K'], s[3]['r'], s[3]['C'], s[3]['X'], s[3]['Z']))
    print('VALIDATION', 'PASS' if ok else 'FAIL')
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
