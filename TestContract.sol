pragma solidity ^0.8.0;
// SPDX-License-Identifier: UNLICENSED

contract TestContract {
    ///////// DECODE EVENTS

    event Transfer(address indexed from, address indexed to, uint value);




    ///////// DECODE FUNCTION RESULT

    function decode_flat1() public returns (
        uint o1
    ) {}

    function decode_flat2() public returns (
        uint o1,
        address o2,
        bytes32 o3,
        bytes12 o4,
        bool o5
    ) {}


    function decode_static_array() public returns (
        uint[3] memory o1
    ) {}

    function decode_nested_static_array() public returns (
        uint[3][2] memory o1
    ) {}



    struct DecodeStruct {
        uint a;
        bool[4] b;
    }

    function decode_struct() public returns (
        DecodeStruct memory o1
    ) {}


    struct DecodeStruct2 {
        uint a;
        bool b;
    }

    function decode_fixed_array_of_structs() public returns (
        DecodeStruct2[3] memory o1
    ) {}


    function decode_dyn_array_of_structs() public returns (
        DecodeStruct2[] memory o1
    ) {}




    struct DecodeStruct3 {
        uint a;
        string b;
    }

    function decode_fixed_array_of_dyn_structs() public returns (
        DecodeStruct3[2] memory o1
    ) {}

    function decode_dyn_array_of_dyn_structs() public returns (
        DecodeStruct3[] memory o1
    ) {}

    function decode_multi_dyn_array_of_dyn_structs() public returns (
        DecodeStruct3[][] memory o1
    ) {}



    function decode_multi3() public returns (
        uint[2][][][] memory o1
    ) {}



    struct DecodeNestedStruct1 {
        address addr;
    }

    struct DecodeNestedStruct2 {
        uint a;
        DecodeNestedStruct1 b;
        bool c;
    }

    function decode_nested_structs_static() public returns (
        DecodeNestedStruct2 memory o1
    ) {}




    struct DecodeNestedStruct3 {
        string myStr;
    }

    struct DecodeNestedStruct4 {
        uint a;
        DecodeNestedStruct3 b;
        bool c;
    }

    function decode_nested_structs_dyn() public returns (
        DecodeNestedStruct4 memory o1
    ) {}




    struct DecodeStructArr1 {
        uint[4] a;
    }

    function decode_structs_arr_static() public returns (
        DecodeStructArr1 memory o1
    ) {}



    struct DecodeStructArr2 {
        string[4] a;
    }

    function decode_structs_arr_dyn() public returns (
        DecodeStructArr2 memory o1
    ) {}

    function decode_structs_arr2_dyn() public returns (
        DecodeStructArr2[] memory o1
    ) {}

    function decode_structs_arr3_dyn() public returns (
        DecodeStructArr2[] memory o1,
        DecodeStructArr2[] memory o2
    ) {}



    struct DecodeStructArr3 {
        string[] a;
    }

    function decode_structs_arr_dyn_dyn() public returns (
        DecodeStructArr3 memory o1
    ) {}







    struct MyNestedStruct {
        address addr;
        uint[] nums;
    }

    struct MyDynStruct {
        uint a;
        uint b;
        string str;
        MyNestedStruct nested;
    }

    struct MyStaticStruct {
        uint[2] s1;
        address s2;
        bytes32 s3;
    }

    function decode_kitchenSink() public returns (
        uint o1,
        uint[] memory o2,
        uint o3,
        uint[4] memory o3_5,
        string memory o4,
        bytes memory o5,
        bytes32 o6,
        bytes12 o7,
        MyDynStruct memory o8,
        bool o9,
        MyStaticStruct memory o10
    ) {}






    ///////// ENCODE FUNCTION DATA

    function encode_flat1(
        uint p1,
        int32 p2,
        bytes32 p3,
        address p4
    ) public {}



    function encode_string(
        uint p1,
        string calldata p2,
        uint p3
    ) public {}



    struct EncodeStruct1 {
        uint a;
        bool b;
        bool c;
    }

    function encode_struct1(
        uint p1,
        EncodeStruct1 calldata p2,
        uint p3
    ) public {}

    function encode_fixed_arr_struct1(
        uint p1,
        EncodeStruct1[2] calldata p2,
        uint p3
    ) public {}

    function encode_dyn_arr_struct1(
        uint p1,
        EncodeStruct1[] calldata p2,
        uint p3
    ) public {}




    struct EncodeStruct2 {
        uint a;
        string b;
    }

    function encode_struct2(
        uint p1,
        EncodeStruct2 calldata p2,
        uint p3
    ) public {}

    function encode_fixed_arr_struct2(
        uint p1,
        EncodeStruct2[2] calldata p2,
        uint p3
    ) public {}



    struct EncodeStruct3 {
        EncodeStruct2[4] a;
    }

    function encode_struct3(
        EncodeStruct3 calldata p1,
        string calldata p2
    ) public {}

    function encode_struct3_2(
        EncodeStruct3[2] calldata p1
    ) public {}

    function encode_struct3_3(
        EncodeStruct3[] calldata p1
    ) public {}

    function encode_struct3_4(
        EncodeStruct3[][] calldata p1
    ) public {}

    function encode_struct3_5(
        EncodeStruct3[2][] calldata p1
    ) public {}



    function encode_kitchenSink(
        int p3,
        int192 p4,
        string calldata p6,
        bytes calldata p7,
        bytes32 p8,
        bytes12 p9,
        uint[] calldata p10,
        address p11
    ) public {}





    function encode_int_limits(
        int p1,
        int32 p2,
        uint128 p3
    ) public {}



}
