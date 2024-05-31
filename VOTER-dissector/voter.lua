-- VOTER Protocol Dissector for Wireshark
-- Created by Mason Nelson, N5LSN/WRKF394

-- The VOTER Protocol – Voice Observing Time Extension (for) Radio
-- was created by Jim Dixon, WB6NIL (SK). This protocol is used for
-- transmission of audio signals from remote radio receivers along with
-- ultra-precise (GPS-based) timing information and signal quality.

-- Define the VOTER protocol
voter_proto = Proto("voter", "VOTER Protocol")

-- Define the fields of the VOTER protocol
local fields = voter_proto.fields
fields.seconds = ProtoField.uint32("voter.seconds", "Seconds", base.DEC)
fields.nanoseconds_or_seq = ProtoField.uint32("voter.nanoseconds_or_seq", "Nanoseconds/Sequence", base.DEC)
fields.auth_challenge = ProtoField.string("voter.auth_challenge", "Auth Challenge")
fields.auth_response = ProtoField.uint32("voter.auth_response", "Auth Response", base.HEX)
fields.payload_type = ProtoField.uint16("voter.payload_type", "Payload Type", base.DEC)
fields.flags = ProtoField.uint8("voter.flags", "Flags", base.HEX)
fields.rssi = ProtoField.uint8("voter.rssi", "RSSI", base.DEC)
fields.mu_law_audio = ProtoField.bytes("voter.mu_law_audio", "Mu-law Audio")
fields.longitude = ProtoField.string("voter.longitude", "Longitude")
fields.latitude = ProtoField.string("voter.latitude", "Latitude")
fields.elevation = ProtoField.string("voter.elevation", "Elevation")
fields.adpcm_audio = ProtoField.bytes("voter.adpcm_audio", "ADPCM Audio")
fields.ping_payload = ProtoField.bytes("voter.ping_payload", "Ping Payload")

-- Dissector function
function voter_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = voter_proto.name

    -- Parse the common header
    local seconds = buffer(0, 4):uint()
    local nanoseconds_or_seq = buffer(4, 4):uint()
    local payload_type = buffer(22, 2):uint()

    -- Set the info column text based on payload type
    local info_text
    if payload_type == 0 then
        info_text = string.format("[0 - Auth + Flags] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    elseif payload_type == 1 then
        info_text = string.format("[1 - RSSI + Mu-law] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    elseif payload_type == 2 then
        info_text = string.format("[2 - GPS] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    elseif payload_type == 3 then
        info_text = string.format("[3 - RSSI + ADPCM] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    elseif payload_type == 4 then
        info_text = string.format("[4 - RESERVED] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    elseif payload_type == 5 then
        info_text = string.format("[5 - PING] Seconds: %d, Nanoseconds: %d", seconds, nanoseconds_or_seq)
    else
        info_text = string.format("[Unknown Payload Type: %d] Seconds: %d, Nanoseconds: %d", payload_type, seconds, nanoseconds_or_seq)
    end
    pinfo.cols.info = info_text

    -- Create a subtree for VOTER protocol
    local subtree = tree:add(voter_proto, buffer(), "VOTER Protocol Data")

    -- Add fields to the subtree
    subtree:add(fields.payload_type, buffer(22, 2))
    subtree:add(fields.seconds, buffer(0, 4))
    subtree:add(fields.nanoseconds_or_seq, buffer(4, 4))
    subtree:add(fields.auth_challenge, buffer(8, 10))
    subtree:add(fields.auth_response, buffer(18, 4))

    -- Switch based on payload type
    if payload_type == 0 then
        -- Authentication plus flags
        subtree:add(fields.flags, buffer(24, 1))
    elseif payload_type == 1 then
        -- RSSI information + Mu-law audio
        subtree:add(fields.rssi, buffer(24, 1))
        subtree:add(fields.mu_law_audio, buffer(25, 160))
    elseif payload_type == 2 then
        -- GPS Information
        local gps_len = buffer:len() - 24
        if gps_len > 0 then
            subtree:add(fields.longitude, buffer(24, 9))
            subtree:add(fields.latitude, buffer(33, 10))
            subtree:add(fields.elevation, buffer(43, 7))
        end
    elseif payload_type == 3 then
        -- RSSI information + IMA ADPCM audio
        subtree:add(fields.rssi, buffer(24, 1))
        subtree:add(fields.adpcm_audio, buffer(25, 163))
    elseif payload_type == 5 then
        -- Ping (connectivity test)
        local ping_len = buffer:len() - 24
        if ping_len > 0 then
            subtree:add(fields.ping_payload, buffer(24, ping_len))
        end
    else
        subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "Unknown payload type")
    end
end

-- Register the dissector for a specific UDP port (667)
local udp_port = DissectorTable.get("udp.port")
udp_port:add(667, voter_proto)