const ethers = require('ethers');
const fs = require('fs');
const child_process = require('child_process');
const {expect} = require('chai');
const canonicalJsonStringify = require('json-stable-stringify');



let abi = JSON.parse(fs.readFileSync('./artifacts/TestContract.abi', 'utf8'));
let interface = new ethers.utils.Interface(abi);





////////////// DECODE LOGS

{
    let format = interface.getEvent('Transfer');
    let topics = ethers.utils.hexlify(ethers.utils.concat([
        ethers.utils.keccak256(Buffer.from(interface.getEvent('Transfer').format())),
        ethers.utils.hexZeroPad("0x1111111111111111111111111111111111111111", 32),
        ethers.utils.hexZeroPad("0x2222222222222222222222222222222222222222", 32),
    ]));

    let data = "0x3333333333333333333333333333333333333333333333333333333333333333";

    let res = JSON.parse(child_process.execSync(`./testHarness decodeEvent ${topics} ${data}`).toString());

    expect(res).to.deep.equal({
        name: 'Transfer',
        args: {
            from: '0x1111111111111111111111111111111111111111',
            to: '0x2222222222222222222222222222222222222222',
            value: ethers.BigNumber.from(data).toString(),
        },
    });
}





////////////// DECODE FUNCTION RESULT

decodeFunctionResult('decode_flat1', {
    o1: "5555555",
});

decodeFunctionResult('decode_flat2', {
    o1: "5555555",
    o2: "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    o3: "0x3333333333333333333333333333333333333333333333333333333333333333",
    o4: "0x007777777777777777777700",
    o5: true,
});



decodeFunctionResult('decode_static_array', {
    o1: ["100", "200", "300"],
});

decodeFunctionResult('decode_nested_static_array', {
    o1: [
        ["100", "200", "300"],
        ["500", "600", "700"],
    ],
});



decodeFunctionResult('decode_struct', {
    o1: {
        a: "1234",
        b: [true, true, false, true],
    },
});



decodeFunctionResult('decode_fixed_array_of_structs', {
    o1: [
        { a: "123", b: false, },
        { a: "923", b: true, },
        { a: "982", b: false, },
    ],
});




decodeFunctionResult('decode_dyn_array_of_structs', {
    o1: [],
});

decodeFunctionResult('decode_dyn_array_of_structs', {
    o1: [
        { a: "123", b: false, },
        { a: "923", b: true, },
        { a: "982", b: false, },
    ],
});



decodeFunctionResult('decode_fixed_array_of_dyn_structs', {
    o1: [
        { a: "123", b: "hello", },
        { a: "923", b: "world", },
    ],
});


decodeFunctionResult('decode_dyn_array_of_dyn_structs', {
    o1: [
        { a: "123", b: "hello", },
        { a: "923", b: "world", },
    ],
});


decodeFunctionResult('decode_multi_dyn_array_of_dyn_structs', {
    o1: [
            [
                { a: "123", b: "hello", },
                { a: "923", b: "world", },
            ],
            [],
            [
                { a: "192839283", b: "akdjfwoifaioefwi", },
            ],
        ],
});




decodeFunctionResult('decode_multi3', {
    o1: [
            [
                [
                    ["1234","2345"],
                    ["1234","2345"],
                ],
                [],
            ],
            [
                [
                    ["66666","777777"],
                ],
            ],
        ],
});



decodeFunctionResult('decode_nested_structs_static', {
    o1: {
        a: "983249283",
        b: {
            addr: "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        },
        c: true,
    },
});




decodeFunctionResult('decode_nested_structs_dyn', {
    o1: {
        a: "983249283",
        b: {
            myStr: "my string goes here goes here goes here goes here goes here goes here goes here goes here goes here",
        },
        c: true,
    },
});




decodeFunctionResult('decode_structs_arr_static', {
    o1: {
        a: ["1", "2", "3", "4"],
    },
});




decodeFunctionResult('decode_structs_arr_dyn', {
    o1: {
        a: ["1", "2", "3", "4"],
    },
});

decodeFunctionResult('decode_structs_arr2_dyn', {
    o1: [
        {
            a: ["1", "2", "3", "4"],
        },
        {
            a: ["1", "2", "3", "4"],
        },
        {
            a: ["1", "2", "3", "4"],
        },
    ],
});

decodeFunctionResult('decode_structs_arr3_dyn', {
    o1: [
        {
            a: ["1", "2", "3", "4"],
        },
        {
            a: ["1", "2", "3", "4"],
        },
    ],
    o2: [
        {
            a: ["1", "2", "3", "4"],
        },
    ],
})
decodeFunctionResult('decode_structs_arr_dyn_dyn', {
    o1: {
        a: ["1", "2", "3", "4"],
    },
});



decodeFunctionResult('decode_kitchenSink', {
    o1: "5555555",
    o2: ["1", "2", "3"],
    o3: "8888888",
    o3_5: ["10", "20", "30", "40"],
    o4: "hello world",
    o5: "0x12345678",
    o6: "0x3333333333333333333333333333333333333333333333333333333333333333",
    o7: "0x007777777777777777777700",
    o8: {
        a: "12345",
        b: "9876654",
        str: "this is a string!",
        nested: {
            addr: "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            nums: ["8000", "9000"],
        },
    },
    o9: true,
    o10: {
        s1: ["123", "234"],
        s2: "0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        s3: "0x4444444444444444444444444444444444444444444444444444444444444444",
    },
});





////////////// ENCODE FUNCTION DATA

encodeFunctionData('encode_flat1', {
    p1: 10000000000,
    p2: -500,
    p3: "0x3333333333333333333333333333333333333333333333333333333333333333",
    p4: "0x2222222222222222222222222222222222222222",
});


encodeFunctionData('encode_string', {
    p1: 1234,
    p2: "hello world!",
    p3: 4321,
});



encodeFunctionData('encode_struct1', {
    p1: 1234,
    p2: {
        a: 9999,
        b: true,
        c: false,
    },
    p3: 4321,
});


encodeFunctionData('encode_fixed_arr_struct1', {
    p1: 1234,
    p2: [
            {
                a: 9999,
                b: true,
                c: false,
            },
            {
                a: 192391293,
                b: false,
                c: false,
            },
    ],
    p3: 4321,
});


encodeFunctionData('encode_dyn_arr_struct1', {
    p1: 1234,
    p2: [
            {
                a: 9999,
                b: true,
                c: false,
            },
            {
                a: 192391293,
                b: false,
                c: false,
            },
    ],
    p3: 4321,
});







encodeFunctionData('encode_struct2', {
    p1: 1234,
    p2: {
        a: 9999,
        b: "hello world!",
    },
    p3: 4321,
});


encodeFunctionData('encode_fixed_arr_struct2', {
    p1: 1234,
    p2: [
        {
            a: 9999,
            b: "hello world!",
        },
        {
            a: 9999,
            b: "hello world!",
        },
    ],
    p3: 4321,
});



encodeFunctionData('encode_struct3', {
    p1: {
        a: [
            { a: 88, b: "hello!", },
            { a: 78, b: "hello2!", },
            { a: 68, b: "hello3!", },
            { a: 58, b: "hello4!", },
        ],
    },
    p2: "this is a really really really really really really really really really really really really really really really long string",
});


encodeFunctionData('encode_struct3_2', {
    p1: [
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
    ],
});


encodeFunctionData('encode_struct3_3', {
    p1: [
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
    ],
});



encodeFunctionData('encode_struct3_5', {
    p1: [[
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
    ], [
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
            {
                a: [
                    { a: 88, b: "hello!", },
                    { a: 78, b: "hello2!", },
                    { a: 68, b: "hello3!", },
                    { a: 58, b: "hello4!", },
                ],
            },
    ]]
});




encodeFunctionData('encode_kitchenSink', {
    p3: '-19231212939123912939',
    p4: '-123123123123',
    p6: "hello world!",
    p7: "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
    p8: "0x3333333333333333333333333333333333333333333333333333333333333333",
    p9: "0x0011223344556677889900aa",
    p10: [1,2,3],
    p11: "0x00aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
});





encodeFunctionData('encode_int_limits', {
    p1: '1000',
    p2: '-1231233',
    //p3: '340282366920938463463374607431768211455',
    p3: 50,
});





console.log("All OK.");




function encodeFunctionData(funcName, args) {
    let argsArray;

    if (Array.isArray(args)) {
        argsArray = args;
    } else {
        argsArray = [];
        // Tests must list keys in same order as defined in solidity
        for (let k of Object.keys(args)) argsArray.push(args[k]);
    }

    let expectedEncoded = interface.encodeFunctionData(funcName, argsArray);
    let expected = interface.decodeFunctionData(funcName, expectedEncoded);

    //console.log("EXPECTED:");
    //dumpWords(expectedEncoded.substr(8));

    let resEncoded = child_process.execSync(`./testHarness encodeFunctionData ${funcName} '${JSON.stringify(args)}'`).toString().trimEnd();
    let res = interface.decodeFunctionData(funcName, resEncoded);

    //console.log("GOT:");
    //dumpWords(resEncoded.substr(8));

    expect(canonicalJsonStringify(cleanupObj(expected))).to.equal(canonicalJsonStringify(cleanupObj(res)));
}


function decodeFunctionResult(funcName, args) {
    let argsArray;

    if (Array.isArray(args)) {
        argsArray = args;
    } else {
        argsArray = [];
        for (let k of Object.keys(args)) argsArray.push(args[k]);
    }

    let encodedResult = interface.encodeFunctionResult(funcName, argsArray);
    //console.log("ENC RES");
    //dumpWords(encodedResult);

    let res = child_process.execSync(`./testHarness decodeFunctionResult ${funcName} ${encodedResult}`).toString().trimEnd();

    expect(res).to.equal(canonicalJsonStringify(args));
}



function dumpWords(words) {
    words = words.substr(2);
    let i = 0;

    while (words.length > 0) {
        console.log(i.toString(16).padStart(4, '0'), words.substr(0, 64));
        words = words.substr(64);
        i+=32;
    }
}

function cleanupObj(obj, decimals) {
    if (obj === null) return obj;

    if (typeof obj === 'object') {
        if (obj._isBigNumber) {
            if (decimals === undefined) return obj.toString();
            else return ethers.utils.formatUnits(obj, decimals);
        }

        if (obj.length === Object.keys(obj).length) {
            return obj.map(o => cleanupObj(o, decimals));
        }

        let ret = {};

        for (let k of Object.keys(obj)) {
            if ('' + parseInt(k) === k) continue;
            ret[k] = cleanupObj(obj[k], decimals);
        }

        return ret;
    }

    return obj;
}
