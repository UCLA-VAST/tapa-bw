puts "applying partitioning constraints"

create_pblock pblock_X4Y0_X7Y3
resize_pblock pblock_X4Y0_X7Y3 -add CLOCKREGION_X4Y0:CLOCKREGION_X6Y3

add_cells_to_pblock [get_pblocks pblock_X4Y0_X7Y3] [get_cells -regex {
  pfm_top_i/dynamic_region/bw
}]
