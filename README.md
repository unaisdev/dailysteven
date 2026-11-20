# Daily Steven

Content pack for [Hortelano's Daily](https://github.com/unaisdev/hortelanos-daily): one shared doubles battle a day against **Steven** and **Aura** or **Blasco**.

The repo folder and the ROM magic strings stay `dailymaximo` / `DAILYMAX-*`. The product name, the daily seed (`dailysteven:`) and the names spoken in battle are Daily Steven / STEVEN / AURA / BLASCO.

Built on [Hall of Legends](https://github.com/amoi-builds/halloflegends) / [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion), with the same browser + mGBA flow as Daily Red / Daily Gold.

## Windows + WSL: un solo comando

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1          # compila (unos segundos si solo cambias textos o C)
powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1 -Clean   # regenera todo desde cero
node scripts/serve.mjs                                                    # http://localhost:8080 (recarga con Ctrl+F5)
```

Sincroniza la carpeta con una copia en WSL, regenera la expansion si hace falta, compila y deja la ROM en
`web/game/dailymaximo.gba.gz` más `web/game/BUILD-INFO.txt` (hora y huella de lo que hay compilado).
Todos los textos de la escena y del final (y los nombres STEVEN/AURA/BLASCO/HORTE) están en
`overlay/include/dailymax_texts.h`; los equipos provisionales de los rivales, en `overlay/data/dailymax_rivals.party`.

ROM de pruebas (`-Debug`, no se publica): `build-wsl.ps1 -Debug` crea `web/game/dailymaximo-debug.gba.gz`. Se juega con
`http://localhost:8080/?debug=1&seed=900001` (tu equipo son 6 Magikarp: derrota segura) o `...&seed=900002` (los rivales
empiezan con 1 PS: victoria segura). `tests/teams/` vuelca y verifica los equipos del generador frente a las reglas de Daily Red.

Al compilar se ejecutan `scripts/check-names.py` (nombres y textos en castellano) y `scripts/check-gen3.py`
(nada de 4.ª gen): si algo falla, no se compila.

## Play locally

```sh
scripts/get-expansion.sh   # pokeemerald-expansion + HoL + Daily Steven overlay + castellano (locale/es)
scripts/build.sh           # or on Windows: bash scripts/_docker_build.sh
scripts/fetch-vendor.sh
node scripts/serve.mjs
```

Castellano uses the same Acimut `pret_europe` overlays as [Daily Gold](https://github.com/unaisdev/dailygold) (`locale/es/` + `scripts/apply-locale.sh`).

On Windows without a native ARM toolchain, use Docker Desktop:

```sh
bash scripts/_docker_build.sh
bash scripts/_package_web.sh   # if build ran make directly
scripts/fetch-vendor.sh
node scripts/serve.mjs
```

Open http://localhost:8080. Practice with `/?seed=1` (Aura) and `/?seed=2` (Blasco).

## How it works

1. The site hashes `dailysteven:YYYY-MM-DD` → seed and injects `DAILYMAX-CONF-V1` into the ROM.
2. The game generates a level-100 team of six and warps onto the champion stage (no door walk-in). Bruno’s room and the door cinematic are the target scene; they are not in this build yet.
3. The daily fight starts immediately: you alone vs Steven’s 3 + Aura or Blasco (`seed & 1`)’s 3. Each partner has one fixed team for now.
4. Result is written to `DAILYMAX-STATUS1` in RAM for the site.

See `docs/status-al-entrar.md` for optional status-on-entry notes.

Spec gap analysis (Daily Steven DOCX + independent review vs this repo): [`docs/gap-analisis-daily-steven.md`](docs/gap-analisis-daily-steven.md). Full spec + Anexo A: [`docs/Daily_Steven_instrucciones_y_revision_independiente.md`](docs/Daily_Steven_instrucciones_y_revision_independiente.md).

## Credits

Daily Steven stands on the work of others (full text in [content/licenses.txt](content/licenses.txt), which also ships inside the content pack):

- **amoi-builds**: [Daily Red](https://github.com/amoi-builds/dailyred) (the rules and the one-battle-a-day format) and [Hall of Legends](https://github.com/amoi-builds/halloflegends) (the base this game is built on).
- **RHH**: [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion) (commit `8fb02c6`) and its [contributors](https://github.com/rh-hideout/pokeemerald-expansion/blob/master/CREDITS.md).
- **pret**: the [pokeemerald and pokefirered decompilations](https://github.com/pret). The player's TM/HM compatibility and level-up moves are Pokémon Emerald's, from pret/pokeemerald.
- **Acimut**: Spanish text tables ([Acimut/pokefirered](https://github.com/Acimut/pokefirered), branch `pret_europe`).
- **Trainer sprites** (via [The Spriters Resource](https://www.spriters-resource.com/), as credited by Hall of Legends): Ethan, FourLeafSunny; Cynthia, lichen; Silver, Nx-Kun, mufasakong, Dragoon; Dawn, Kaori; Barry, Gridiron, Eevee, Zhen Lin, pika, Pipian.
- **Browser player**: [Nostalgist.js](https://github.com/arianrhodsandlot/nostalgist) and [mGBA](https://github.com/mgba-emu/mgba)'s libretro core; see [web/licenses.txt](web/licenses.txt).

## Publishing a content pack (by hand, never on push)

1. Write 8 bullets at most in `content/NOVEDADES.md` (player-facing Spanish, no jargon).
2. Make sure the launcher (`unaisdev/hortelanos-daily`) already ships the `dailymax-conf-v1+status-v1` adapter.
3. Run the **Release content pack** workflow by hand (Actions → Run workflow → version `X.Y.Z`), or push a `content-vX.Y.Z` tag on purpose. It builds the ROM from scratch, runs the build validators and the Gen3 mechanics tests, packages the pack and creates a **draft** release. Nothing is public until someone publishes that draft.
4. Afterwards, empty `content/NOVEDADES.md` in a normal PR (`scripts/clear-novedades.sh`). There are no automatic commits.

Local pack (no GitHub): build the ROM, then `scripts/package-content.sh` writes `dist/content/dailysteven-content.zip` and `SHA256SUMS`.

## Disclaimer

Fan project, not affiliated with Nintendo, Game Freak or The Pokémon Company.
