"""
PROJECT 1864 - building costs and build times, c. 1851 (design proposal).

One model for every building so the numbers stay consistent: each building has a size class
(labour in man-days and the largest useful crew) and a construction type (materials per 1,000
man-days). Money = labour + materials + special items (imported machinery, guns, lanterns).
Time = man-days / crew, plus delivery time for imported items.

Money is in rigsdaler (rd., 96 skilling). The prices are game estimates at an 1850s price level,
not researched historical figures. Change the constants below and rerun:

    python Tools/Campaign/building_costs.py

Writes Data/Campaign1851/Buildings1851.csv (read by the game later) and
Docs/Buildings1851-Costs.md (the table for review).
"""
import csv
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))

# Size class -> (man-days, largest useful crew).
SIZES = {
    "XS": (800, 15),
    "S": (3000, 40),
    "M": (10000, 80),
    "L": (30000, 200),
    "XL": (90000, 400),
    "XXL": (250000, 800),
}

# Construction type -> materials per 1,000 man-days: timber (loads), building materials
# (brick/stone/lime, loads), iron (centner, 50 kg); and the wage per man-day (rd.).
TYPES = {
    "bindingsværk": {"timber": 28, "stone": 12, "iron": 4, "wage": 1.0},   # timber frame
    "grundmur": {"timber": 12, "stone": 45, "iron": 8, "wage": 1.0},       # brick
    "industri": {"timber": 12, "stone": 40, "iron": 30, "wage": 1.0},      # brick + ironwork, kilns, shafts
    "jordværk": {"timber": 6, "stone": 6, "iron": 2, "wage": 0.8},         # earthworks, mostly unskilled
    "stenværk": {"timber": 10, "stone": 60, "iron": 10, "wage": 1.1},      # cut stone: bridges, quays, towers
}

PRICES = {"timber": 12.0, "stone": 16.0, "iron": 6.0}   # rd. per load / load / centner

# Upkeep per year as a share of the building cost, by who runs it.
UPKEEP = {"Stat (militær)": 0.04, "Stat": 0.03, "By": 0.02, "Privat": 0.0}

# (key, Danish name, category, image, who builds, size, type, special rd., special days, special note, requires, provides)
BUILDINGS = [
    # --- garrison complex (in the game now)
    ("Garrison_Barracks", "Infanterikaserne", "Garnison", "1851_Military_Infantry_Barracks", "Stat (militær)", "L", "grundmur", 0, 0, "",
     "Købstad over 2.500 indb. med garnisonsgrund", "Indkvartering af 1 bataljon (ca. 800 mand)"),
    ("Garrison_Stables", "Stalde", "Garnison", "1851_Military_Remount_Depot", "Stat (militær)", "M", "grundmur", 0, 0, "",
     "Infanterikaserne", "200 heste opstaldet ved garnisonen"),
    ("Garrison_Depot", "Depot og magasin", "Garnison", "1851_Agriculture_Grain_Warehouse", "Stat (militær)", "M", "grundmur", 0, 0, "",
     "Infanterikaserne", "Mobiliseringslager til 1 bataljon (uniformer, våben, feltudstyr)"),
    ("Garrison_Infirmary", "Sygestue", "Garnison", "1851_Military_Field_Hospital", "Stat (militær)", "S", "grundmur", 0, 0, "",
     "Infanterikaserne", "60 senge til garnisonen"),
    # --- military
    ("Mobilization_Center", "Mobiliseringsdepot", "Militær", "1851_Military_Mobilization_Center", "Stat (militær)", "L", "grundmur", 0, 0, "",
     "Infanterikaserne", "Samling og udrustning af reserver"),
    ("Remount_Depot", "Remontedepot", "Militær", "1851_Military_Remount_Depot", "Stat (militær)", "L", "bindingsværk", 0, 0, "",
     "Landområde med græsning", "Modtagelse og fordeling af 600 erstatningsheste"),
    ("Officer_School", "Officersskole", "Militær", "1851_Military_Officer_School", "Stat (militær)", "L", "grundmur", 0, 0, "",
     "By over 10.000 indb.", "Uddannelse af officerer (langsigtet)"),
    ("Arsenal", "Arsenal", "Militær", "1851_Military_Artillery_Arsenal_01", "Stat (militær)", "XL", "industri", 0, 0, "",
     "By over 10.000 indb. med havn", "Lager, reparation og udlevering af materiel"),
    ("Cannon_Foundry", "Kanonstøberi", "Militær", "1851_Military_Artillery_Works_Cannon_Foundry", "Stat (militær)", "XL", "industri", 40000, 90,
     "Boreværk og ovne (import)", "Maskinværksted; adgang til jern og kul", "Nye kanoner og lavetter"),
    ("Rifle_Workshop", "Geværværksted", "Militær", "1851_Military_Rifle_Workshop", "Stat (militær)", "M", "industri", 15000, 60,
     "Værktøjsmaskiner (import)", "Maskinværksted", "Nye og ombyggede geværer"),
    ("Ammunition_Works", "Ammunitionsfabrik", "Militær", "1851_Military_Ammunition_Works", "Stat (militær)", "M", "industri", 8000, 30,
     "Presser og støbeforme", "Krudtværk eller krudtmagasin i nærheden", "Patroner og granater"),
    ("Gunpowder_Works", "Krudtværk", "Militær", "1851_Military_Gunpowder_Works", "Stat (militær)", "L", "industri", 10000, 30,
     "Stampe- og kornværk", "Vandløb; mindst 2 km fra by", "Krudt"),
    ("Powder_Magazine", "Krudtmagasin", "Militær", "1851_Military_Powder_Magazine", "Stat (militær)", "S", "jordværk", 0, 0, "",
     "Uden for byen", "Sikkert krudtlager"),
    ("Coastal_Battery", "Kystbatteri", "Militær", "1851_Military_Coastal_Battery", "Stat (militær)", "M", "jordværk", 12000, 0,
     "6 kystkanoner fra arsenalet", "Kyst ved sejlløb", "Forsvar af havn eller sejlløb"),
    ("Star_Fort", "Skanse / fæstningsværk", "Militær", "1851_Military_Fortification_Star_Fort", "Stat (militær)", "XXL", "jordværk", 30000, 0,
     "Palisader, blokhuse og kanoner", "Strategisk punkt (fx Dannevirke, Dybbøl, Fredericia)", "Permanent forsvarsstilling"),
    ("Field_Hospital", "Lazaret", "Sundhed", "1851_Military_Field_Hospital", "Stat (militær)", "M", "grundmur", 0, 0, "",
     "By over 2.500 indb.", "200 senge; hurtigere helbredelse af sårede"),
    ("Hospital", "Sygehus", "Sundhed", "1851_Medical_Hospital", "By", "L", "grundmur", 0, 0, "",
     "By over 5.000 indb.", "300 senge; civil og militær behandling"),
    # --- civic
    ("Town_Hall", "Rådhus", "Civil", "1851_Civic_Town_Hall", "By", "M", "grundmur", 0, 0, "",
     "Købstad", "Administration og ro i byen"),
    ("Courthouse", "Tinghus / domhus", "Civil", "1851_Civic_Courthouse", "Stat", "M", "grundmur", 0, 0, "",
     "Amtsby", "Retspleje og orden i amtet"),
    ("Post_Office", "Posthus", "Civil", "1851_Civic_Post_Office", "Stat", "S", "grundmur", 0, 0, "",
     "Købstad", "Post og rapporter hurtigere"),
    ("Schoolhouse", "Skole", "Civil", "1851_Civic_Schoolhouse", "By", "S", "grundmur", 0, 0, "",
     "", "Uddannelse (langsigtet)"),
    ("Telegraph_Office", "Telegrafstation", "Infrastruktur", "1851_Infrastructure_Telegraph_Office", "Stat", "S", "grundmur", 4000, 30,
     "Apparater og batterier (plus ledning pr. km)", "Fra 1854 (den første danske telegraf)", "Øjeblikkelige ordrer og rapporter"),
    # --- infrastructure
    ("Railway_Station", "Jernbanestation", "Infrastruktur", "1851_Infrastructure_Railway_Station", "Stat", "M", "grundmur", 0, 0, "",
     "Jernbanelinje (1851: kun København-Roskilde)", "Lastning og losning af tog"),
    ("Freight_Depot", "Godsbanegård", "Infrastruktur", "1851_Infrastructure_Freight_Depot", "Stat", "M", "industri", 0, 0, "",
     "Jernbanestation", "Større godskapacitet på banen"),
    ("Railway_Gatehouse", "Banevogterhus", "Infrastruktur", "1851_Infrastructure_Railway_Gatehouse", "Stat", "XS", "grundmur", 0, 0, "",
     "Jernbanelinje", "Sikker overkørsel og banevogter"),
    ("Stone_Bridge", "Stenbro", "Infrastruktur", "1851_Infrastructure_Stone_Bridge", "Stat", "M", "stenværk", 0, 0, "",
     "Vej over å", "Fast overgang; tåler tung trafik"),
    # --- port
    ("Harbor_Building", "Toldbod / havnekontor", "Havn", "1851_Port_Harbor_Building", "Stat", "S", "grundmur", 0, 0, "",
     "Havneby", "Told og havneadministration"),
    ("Harbor_Warehouse", "Havnepakhus", "Havn", "1851_Port_Harbor_Warehouse", "By", "M", "grundmur", 0, 0, "",
     "Havneby", "Lager ved kajen (import og eksport)"),
    ("Lighthouse", "Fyrtårn", "Havn", "1851_Port_Lighthouse", "Stat", "S", "stenværk", 3000, 45,
     "Linseapparat (import)", "Kyst", "Sikrere sejlads om natten"),
    ("Shipyard", "Skibsværft", "Havn", "1851_Port_Shipyard", "Privat", "XL", "bindingsværk", 20000, 0,
     "Beddinger og kraner", "Havneby", "Bygning og reparation af skibe"),
    ("Admiralty_Office", "Marinestation", "Havn", "1851_Port_Admiralty_Office", "Stat (militær)", "M", "grundmur", 0, 0, "",
     "Orlogshavn", "Flådens administration og forsyning"),
    # --- industry
    ("Brickworks", "Teglværk", "Industri", "1851_Industry_Brickworks", "Privat", "M", "grundmur", 2000, 0,
     "Ringovn", "Lerforekomst", "Byggematerialer"),
    ("Sawmill", "Savværk", "Industri", "1851_Industry_Sawmill", "Privat", "S", "bindingsværk", 3000, 30,
     "Vandhjul eller dampmaskine", "Skov i amtet", "Tømmer"),
    ("Machine_Workshop", "Maskinværksted", "Industri", "1851_Industry_Machine_Workshop", "Privat", "M", "industri", 12000, 60,
     "Dampmaskine og drejebænke (import)", "By over 10.000 indb.", "Maskinkapacitet og reparationer"),
    ("Wagon_Works", "Vognfabrik", "Industri", "1851_Industry_Wagon_Works", "Privat", "M", "bindingsværk", 0, 0, "",
     "Købstad", "Vogne til hær og civilsamfund"),
    ("Saddlery", "Sadelmageri", "Industri", "1851_Industry_Saddlery_Workshop", "Privat", "S", "bindingsværk", 0, 0, "",
     "Købstad", "Seletøj og sadler"),
    ("Tannery", "Garveri", "Industri", "1851_Industry_Tannery_Leatherworks", "Privat", "S", "bindingsværk", 0, 0, "",
     "Købstad ved vand", "Læder"),
    ("Textile_Mill", "Klædefabrik", "Industri", "1851_Industry_Textile_Mill", "Privat", "L", "industri", 20000, 60,
     "Væve og spindemaskiner (import)", "By over 5.000 indb.", "Klæde til uniformer"),
    ("Brewery", "Bryggeri og brænderi", "Industri", "1851_Industry_Brewery_Distillery", "Privat", "M", "grundmur", 3000, 0,
     "Kobberkedler", "Købstad", "Skatteindtægt og fødevarer"),
    ("Coal_Mine", "Kulmine", "Industri", "1851_Industry_Coal_Mine", "Privat", "L", "industri", 8000, 30,
     "Pumpe og hejseværk", "Kun Bornholm", "Kul"),
    # --- commerce, agriculture, church, estates
    ("Merchant_House", "Købmandsgård", "Handel", "1851_Commerce_Merchant_House_Shop", "Privat", "M", "grundmur", 0, 0, "",
     "Købstad", "Handel og lager"),
    ("Grain_Warehouse", "Kornmagasin", "Landbrug", "1851_Agriculture_Grain_Warehouse", "Stat", "M", "grundmur", 0, 0, "",
     "Købstad eller havn", "Regional kornreserve"),
    ("Stud_Farm", "Stutteri", "Landbrug", "1851_Agriculture_Stud_Farm", "Stat", "L", "bindingsværk", 5000, 0,
     "Avlsheste", "Landområde med græsning", "Avl af heste (virker efter år)"),
    ("Church", "Kirke", "Kirke", "1851_Religious_Church", "By", "L", "grundmur", 0, 0, "",
     "", "Sognets kirke"),
    ("Estate_Mansion", "Herregård", "Bolig", "1851_Residential_Estate_Mansion", "Privat", "L", "grundmur", 0, 0, "",
     "", "Godsets hovedbygning"),
    ("Manor_House", "Proprietærgård", "Bolig", "1851_Residential_Manor_House", "Privat", "M", "grundmur", 0, 0, "",
     "", "Større landbrug"),
    # --- rural (built by the economy, listed for reference)
    ("Barn", "Lade", "Land", "1851_Rural_Barn", "Privat", "S", "bindingsværk", 0, 0, "", "", "Høst- og foderlager"),
    ("Blacksmith", "Smedje", "Land", "1851_Rural_Blacksmith_Forge", "Privat", "XS", "grundmur", 0, 0, "", "", "Beslag og reparationer"),
    ("Cottage", "Husmandssted", "Land", "1851_Rural_Cottage_Thatched", "Privat", "XS", "bindingsværk", 0, 0, "", "", "Bolig for husmand"),
    ("Farmhouse", "Bondegård (firlænget)", "Land", "1851_Rural_Farmhouse", "Privat", "S", "bindingsværk", 0, 0, "", "", "Landbrug"),
    ("Inn", "Kro", "Land", "1851_Rural_Inn_Tavern", "Privat", "S", "bindingsværk", 0, 0, "", "Landevej", "Rasteplads for rejsende og tropper"),
    ("Stable", "Stald", "Land", "1851_Rural_Stable", "Privat", "S", "bindingsværk", 0, 0, "", "", "Heste og kvæg"),
    ("Windmill", "Vindmølle", "Land", "1851_Rural_Windmill", "Privat", "S", "bindingsværk", 1500, 0,
     "Møllehat og kværne", "", "Mel til by og hær"),
]


# Fine-tuning within a size class (man-days, hence money and time); 1.0 when not listed.
FACTORS = {
    "Garrison_Stables": 0.8, "Garrison_Depot": 0.9, "Garrison_Infirmary": 1.2,
    "Mobilization_Center": 0.8, "Remount_Depot": 0.9, "Officer_School": 1.2, "Cannon_Foundry": 1.1,
    "Ammunition_Works": 0.9, "Gunpowder_Works": 0.8, "Powder_Magazine": 1.3, "Hospital": 1.1,
    "Town_Hall": 1.3, "Courthouse": 1.1, "Schoolhouse": 0.7, "Telegraph_Office": 0.5, "Freight_Depot": 0.8,
    "Stone_Bridge": 0.9, "Harbor_Building": 1.2, "Harbor_Warehouse": 1.1, "Lighthouse": 1.4, "Shipyard": 0.8,
    "Admiralty_Office": 1.2, "Brickworks": 0.8, "Wagon_Works": 0.7, "Saddlery": 0.8, "Tannery": 0.9,
    "Brewery": 0.9, "Merchant_House": 0.8, "Stud_Farm": 0.8, "Church": 0.7, "Estate_Mansion": 1.4,
    "Manor_House": 1.2, "Barn": 0.8, "Cottage": 0.8, "Farmhouse": 1.3, "Inn": 1.1, "Stable": 0.7, "Windmill": 0.8,
}


def compute(row):
    key, name, category, image, owner, size, kind, special, special_days, special_note, requires, provides = row
    man_days, crew = SIZES[size]
    man_days = int(man_days * FACTORS.get(key, 1.0))
    t = TYPES[kind]
    timber = round(man_days / 1000 * t["timber"])
    stone = round(man_days / 1000 * t["stone"])
    iron = round(man_days / 1000 * t["iron"])
    labour = man_days * t["wage"]
    materials = timber * PRICES["timber"] + stone * PRICES["stone"] + iron * PRICES["iron"]
    cost = int(round((labour + materials + special) / 100.0) * 100)
    days = int(round(man_days / crew)) + special_days
    upkeep = int(round(cost * UPKEEP[owner] / 10.0) * 10)
    return {
        "key": key, "name": name, "category": category, "owner": owner, "size": size, "type": kind,
        "cost_rd": cost, "days": days, "man_days": man_days, "crew": crew,
        "timber_loads": timber, "stone_loads": stone, "iron_centner": iron,
        "special_rd": special, "special": special_note, "upkeep_rd_year": upkeep,
        "requires": requires, "provides": provides, "image": image,
    }


def thousands(n):
    return f"{n:,}".replace(",", ".")


def main():
    rows = [compute(r) for r in BUILDINGS]
    data = os.path.join(ROOT, "Data", "Campaign1851", "Buildings1851.csv")
    with open(data, "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    lines = [
        "# Bygninger 1851 — pris og byggetid (forslag)",
        "",
        "Genereret af `Tools/Campaign/building_costs.py` — ret konstanterne dér og kør scriptet igen.",
        "Priserne er spilestimater i et 1850'er-prisniveau, ikke historisk efterprøvede tal.",
        "",
        "## Model",
        "",
        "- **Penge** = løn + materialer + særudstyr. Rigsdaler (rd., 96 skilling).",
        f"- **Løn**: {TYPES['grundmur']['wage']:g} rd. pr. mandsdag (jordarbejde {TYPES['jordværk']['wage']:g} rd.).",
        f"- **Materialer**: tømmer {PRICES['timber']:g} rd./læs, byggematerialer (tegl, sten, kalk) {PRICES['stone']:g} rd./læs, jern {PRICES['iron']:g} rd./centner.",
        "- **Tid** = mandsdage / største sjak + leveringstid for importeret udstyr.",
        "- Hver bygning har en finjustering inden for sin størrelse (FACTORS), fx rådhus ×1,3, skole ×0,7.",
        "- **Drift** pr. år: militært 4 %, statsligt 3 %, by 2 % af byggeprisen; private bygninger drives af ejeren.",
        "",
        "| Størrelse | Mandsdage | Største sjak | Grundtid |",
        "| --- | ---: | ---: | ---: |",
    ]
    for s, (md, crew) in SIZES.items():
        lines.append(f"| {s} | {thousands(md)} | {crew} | {round(md / crew)} dage |")
    lines += ["", "| Byggemåde | Tømmer / 1.000 md | Byggemat. / 1.000 md | Jern / 1.000 md |", "| --- | ---: | ---: | ---: |"]
    for k, t in TYPES.items():
        lines.append(f"| {k} | {t['timber']} læs | {t['stone']} læs | {t['iron']} ctr. |")
    lines += ["", "## Bygninger", "",
              "| Bygning | Kategori | Bygherre | Str. | Pris | Tid | Tømmer | Byggemat. | Jern | Drift/år | Kræver | Giver |",
              "| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |"]
    for r in rows:
        special = f" (heraf {thousands(r['special_rd'])} rd. {r['special'].lower()})" if r["special_rd"] else ""
        lines.append(f"| {r['name']} | {r['category']} | {r['owner']} | {r['size']} | {thousands(r['cost_rd'])} rd.{special} | {r['days']} d "
                     f"| {r['timber_loads']} | {r['stone_loads']} | {r['iron_centner']} | {thousands(r['upkeep_rd_year']) if r['upkeep_rd_year'] else '–'} "
                     f"| {r['requires'] or '–'} | {r['provides']} |")
    doc = os.path.join(ROOT, "Docs", "Buildings1851-Costs.md")
    with open(doc, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{len(rows)} buildings -> {data}, {doc}")


if __name__ == "__main__":
    main()
