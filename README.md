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

## Disclaimer

Fan project, not affiliated with Nintendo, Game Freak or The Pokémon Company.
