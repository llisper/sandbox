BEGIN {
   mac_addr = "0C-82-68-44-42-81" 
   dest = "192.30.252.0 mask 255.255.255.0"
}

{
    if ($0 ~ mac_addr) {
        for (i=0; i<8; ++i)
            getline gateway
        split(gateway, arr, ":")
        gateway = arr[2]
        exit 0
    }
}

END {
    system("route add " dest gateway)
}
