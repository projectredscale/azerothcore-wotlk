#!/bin/bash
function MissingConfig {
    echo "mmaps-config.yaml is missing from the current directory."
    echo "The mmaps generator cannot run without it, and extracting mmaps"
    echo "takes hours, so nothing was extracted."
    echo "Copy mmaps-config.yaml next to mmaps_generator and run this again."
    exit 1
}

function Base {
    echo "Extract Base"
    rm -rf dbc maps Cameras
    ./map_extractor
    Menu
}

function VMaps {
    echo "Extract VMaps"
    mkdir -p Buildings vmaps
    rm -rf Buildings/* vmaps/*
    ./vmap4_extractor
    ./vmap4_assembler Buildings vmaps
    rmdir -rf Buildings
    Menu
}

function MMaps {
    [ -f "mmaps-config.yaml" ] || MissingConfig
    echo "This may take a few hours to complete. Please be patient."
    mkdir -p mmaps
    rm -rf mmaps/*
    ./mmaps_generator
    Menu
}

function All {
    [ -f "mmaps-config.yaml" ] || MissingConfig
    echo "This may take a few hours to complete. Please be patient."
    rm -rf dbc maps Cameras
    mkdir -p Buildings vmaps mmaps
    rm -rf Buildings/* vmaps/* mmaps/*
    ./map_extractor
    ./vmap4_extractor
    ./vmap4_assembler Buildings vmaps
    rmdir -rf Buildings
    ./mmaps_generator
    Menu
}

function Menu {
echo ""
echo "..............................................."
echo "AzerothCore dbc, maps, vmaps, mmaps extractor"
echo "..............................................."
echo "PRESS 1, 2, 3 OR 4 to select your task, or 5 to EXIT."
echo "..............................................."
echo ""
echo "WARNING! when extracting the vmaps extractor will"
echo "output the text below, it's intended and not an error:"
echo ".........................................."
echo "Extracting World\Wmo\Band\Final_Stage.wmo"
echo "No such file."
echo "Couldn't open RootWmo!!!"
echo "Done!"
echo " .........................................."
echo ""
echo "Press 1, 2, 3 or 4 to start extracting or 5 to exit."
echo "1 - Extract base files (NEEDED) and cameras."
echo "2 - Extract vmaps (needs maps to be extracted before you run this) (OPTIONAL, highly recommended)"
echo "3 - Extract mmaps (needs vmaps to be extracted before you run this, may take hours) (OPTIONAL, highly recommended)"
echo "4 - Extract all (may take hours)"
echo "5 - EXIT"
echo ""

read -rp "Type 1, 2, 3, 4 or 5 then press ENTER: " choice

case $choice in
    1) Base ;;
    2) VMaps ;;
    3) MMaps ;;
    4) All ;;
    5) exit 0;;
    *) echo "Invalid choice."; read -rp "Type 1, 2, 3, 4 or 5 then press ENTER: " choice ;;
esac
}

if [ -d "./Data" ] && [ -f "map_extractor" ] && [ -f "vmap4_extractor" ] && [ -f "vmap4_assembler" ] && [ -f "mmaps_generator" ] && [ -f "mmaps-config.yaml" ]; then
    echo "The required files and folder exist in the current directory."
    chmod +x map_extractor vmap4_extractor vmap4_assembler mmaps_generator
    Menu
else
    echo "One or more of the required files or folder is missing from the current directory."
    echo "Place map_extractor vmap4_extractor vmap4_assembler mmaps_generator"
    echo "and mmaps-config.yaml in your WoW folder with WoW.exe"
fi
