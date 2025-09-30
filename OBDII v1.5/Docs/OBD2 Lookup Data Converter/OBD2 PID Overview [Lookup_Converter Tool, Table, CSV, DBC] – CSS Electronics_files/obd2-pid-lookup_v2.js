/** Shopify CDN: Minification failed

Line 16:1 Transforming async functions to the configured target environment ("es5") is not supported yet
Line 24:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 28:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 32:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 36:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 43:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 51:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 53:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 55:4 Transforming const to the configured target environment ("es5") is not supported yet
Line 57:4 Transforming const to the configured target environment ("es5") is not supported yet
... and 88 more hidden warnings

**/
(async () => {
    'use strict';
    /**
     * CONSTANTS
     */
    /**
     * How often to update the OBD2 PID list, in days.
     */
    const OBD2_PID_LIST_UPDATE_FREQUENCY = 0;
    /**
     * Where to update the OBD2 PID list from.
     */
    const OBD2_PID_LIST_CSV_URL = 'https://cdn.shopify.com/s/files/1/0579/8032/1980/files/obd2-pid-table-service-01.csv?v=1634294050';
    /**
     * The PID to load by default.
     */
    const DEFAULT_PID = '0D';
    /**
     * Used to validate the inputs.
     */
    const INPUT_REGEXES = {
      hex: /^[A-Za-z0-9]{1,2}$/,
      dec: /^[0-9]{1,3}$/,
    };
    /**
     * Used to color the binaries.
     */
    const BINARY_COLOR_MAP = {
      0: '#f65856',
      1: '#10cba9',
    };
    /**
     * ELEMENT REFERENCES
     */
    /** @type {HTMLSelectElement} */
    const numberFormatSelect = document.querySelector('#dbc-number-format-select');
    /** @type {HTMLSelectElement} */
    const canIdLengthSelect = document.querySelector('#dbc-can-id-length-select');
    /** @type {HTMLSelectElement} */
    const pidSelect = document.querySelector('#dbc-pid-select');
    /** @type {HTMLSelectElement} */
    const nameSelect = document.querySelector('#dbc-name-select');
    const topCells = {
      bit_start: document.querySelector('#bit_start'),
      bit_length: document.querySelector('#bit_length'),
      scale_short: document.querySelector('#scale_short'),
      offset: document.querySelector('#offset'),
      min_max: document.querySelector('#min_max'),
      unit: document.querySelector('#unit'),
    };
    /** @type {HTMLDivElement} */
    const canFrames = document.querySelector('#can_frames');
    /** @type {HTMLTableRowElement} */
    const requestRow = document.querySelector('#dbc-request-row');
    /** @type {HTMLTableRowElement} */
    const responseRow = document.querySelector('#dbc-response-row');
    const [, ...requestCells] = requestRow.cells;
    const [, ...responseCells] = responseRow.cells;
    const responseInputs = [...responseRow.querySelectorAll('input')];
    /** @type {HTMLTableCellElement} */
    const resCalcOffset = document.querySelector('#res_calc_offset');
    /** @type {HTMLTableCellElement} */
    const resCalcScale = document.querySelector('#res_calc_scale');
    /** @type {HTMLTableCellElement} */
    const resCalcData = document.querySelector('#res_calc_data');
    /** @type {HTMLTableCellElement} */
    const resCalcResultDec = document.querySelector('#res_calc_result_dec');
    /** @type {HTMLTableCellElement} */
    const resCalcUnit = document.querySelector('#res_calc_unit');
    /** @type {HTMLTableElement} */
    const resBinTable = document.querySelector('#dbc-res-bin-table');
    /** @type {HTMLTableCellElement} */
    const resCalcResultBin = document.querySelector('#res_calc_result_bin');
    /** @type {HTMLTableElement} */
    const pidsTable = document.querySelector('#dbc-pids-table');
    /** @type {HTMLElement} */
    const pidsBody = document.querySelector('#dbc-pids-body');
    /**
     * VARIABLES
     */
    let oldNumberFormat = 'dec';
    let currentNumberFormat = 'hex';
    let obd2PidList = {};
    /**
     * FUNCTIONS
     */
    /**
     * Updates the OBD2 PID list and stores it in the local storage every specified number of days.
     */
    const updateObd2PidList = async () => {
      const now = Date.now();
      let obj = JSON.parse(window.localStorage.getItem('obd2-pid-list') || '{ "lastUpdate": 0, "list": {} }');
      if (!obj.lastUpdate || now - obj.lastUpdate > OBD2_PID_LIST_UPDATE_FREQUENCY * 86400000) {
        obj.lastUpdate = now;
        try {
          const newList = {};
          const response = await window.fetch(OBD2_PID_LIST_CSV_URL);
          const file = await response.text();
          const [, ...lines] = file.split('\n').filter((line) => line);
          for (const line of lines) {
            const values = [];
            let currentValue = '';
            let previousChar = null;
            let isInsideQuotes = false;
            for (const char of line) {
              if (char === ',') {
                if (previousChar === '"') {
                  isInsideQuotes = false;
                }
                if (isInsideQuotes) {
                  currentValue += char;
                } else {
                  values.push(currentValue);
                  currentValue = '';
                }
              } else if (char === '"') {
                if (previousChar === null || previousChar === ',') {
                  isInsideQuotes = !isInsideQuotes;
                }
                currentValue += char;
              } else {
                currentValue += char;
              }
              previousChar = char;
            }
            if (currentValue) {
              values.push(currentValue);
            }
            if (values[0] === '') {
              continue;
            }
            newList[values[0]] = {
              name_short: values[2],
              name: values[3],
              bit_start: values[4] ? Number.parseInt(values[4]) : null,
              bit_length: values[5] ? Number.parseInt(values[5]) : null,
              scale: values[6] ? Number.parseFloat(values[6]) : null,
              offset: values[7] ? Number.parseFloat(values[7]) : null,
              min: values[8] ? Number.parseFloat(values[8]) : null,
              max: values[9] ? Number.parseFloat(values[9]) : null,
              unit: values[10],
              scale_short: values[11],
              min_max: values[12],
              bit_start_length: values[13],
            };
          }
          obj.list = newList;
        } catch (err) {
          console.error('Failed to update OBD2 PID list', err.message);
        }
        window.localStorage.setItem('obd2-pid-list', JSON.stringify(obj));
      }
      return obj.list;
    };
    /**
     * Changes the current number format.
     */
    const changeNumberFormat = () => {
      oldNumberFormat = currentNumberFormat;
      currentNumberFormat = numberFormatSelect.value;
      updateDropdowns();
      for (const responseInput of responseInputs) {
        responseInput.value = formatNumber(responseInput.value);
        responseInput.nextElementSibling.textContent = formatNumber(responseInput.nextElementSibling.textContent);
      }
      updateTable();
    };
    /**
     * Syncs the dropdowns, resets the response inputs and updates the table.
     *
     * @param {Event} event
     */
    const handleDropdownChange = (event) => {
      if (event.currentTarget === pidSelect) {
        nameSelect.value = pidSelect.value;
      } else {
        pidSelect.value = nameSelect.value;
      }
      responseInputs[0].value = '12';
      responseInputs[1].value = '34';
      responseInputs[2].value = '56';
      responseInputs[3].value = '78';
      updateTable();
    };
    /**
     * Validates a response input to make sure that the value is in the correct range, and updates the table if that's the case.
     *
     * @param {Event} event
     */
    const validateResponseInput = (event) => {
      const regex = INPUT_REGEXES[currentNumberFormat];
      const value = event.target.value;
      if (!regex.test(value)) {
        event.target.setCustomValidity('Value must be between 00 and FF for HEX or 0 and 255 for DEC!');
        event.target.reportValidity();
        return;
      }
      event.target.setCustomValidity('');
      const valueDec = formatNumber(value, currentNumberFormat, 'dec');
      if (valueDec < 0 || valueDec > 255) {
        event.target.setCustomValidity('Value must be between 00 and FF for HEX or 0 and 255 for DEC!');
        event.target.reportValidity();
        return;
      }
      event.target.setCustomValidity('');
      updateTable();
    };
    /**
     * Fills the dropdowns on initial load and is called every time the number format changes to update them.
     */
    const updateDropdowns = () => {
      const selectedIndex = pidSelect.selectedIndex;
      pidSelect.innerHTML = '';
      nameSelect.innerHTML = '';
      const keys = Object.keys(obd2PidList).sort((a, b) => a.localeCompare(b, undefined, {
        numeric: true,
      }));
      for (const key of keys) {
        const keyFormatted = formatNumber(key, 'dec');
        const pidOption = document.createElement('option');
        pidOption.textContent = keyFormatted;
        pidOption.value = keyFormatted;
        pidSelect.appendChild(pidOption);
        const nameOption = document.createElement('option');
        nameOption.textContent = obd2PidList[key].name;
        nameOption.value = keyFormatted;
        nameSelect.appendChild(nameOption);
      }
      pidSelect.selectedIndex = selectedIndex;
      nameSelect.selectedIndex = selectedIndex;
    };
    /**
     * Called every time an input/select changes.
     */
    const updateTable = () => {
      const canIdLength = parseInt(canIdLengthSelect.value);
      const pid = pidSelect.value;
      const pidDec = formatNumber(pid, currentNumberFormat, 'dec');
      const values = obd2PidList[pidDec] || {};
      for (const key of Object.keys(topCells)) {
        topCells[key].textContent = values[key] ?? '';
      }
      if (!values.bit_length) {
        canFrames.style.display = 'none';
        return;
      }
      canFrames.style.display = 'block';
      const dataBytes = responseInputs.map((responseInput) => formatNumber(responseInput.value, currentNumberFormat, 'hex'));
      let data = '';
      requestCells[0].textContent = formatNumber(canIdLength === 11 ? '7DF' : '18DB33F1', 'hex');
      requestCells[1].textContent = formatNumber('02', 'hex');
      requestCells[2].textContent = formatNumber('01', 'hex');
      requestCells[3].textContent = pid;
      requestCells[4].textContent = formatNumber('AA', 'hex');
      requestCells[5].textContent = formatNumber('AA', 'hex');
      requestCells[6].textContent = formatNumber('AA', 'hex');
      requestCells[7].textContent = formatNumber('AA', 'hex');
      requestCells[8].textContent = formatNumber('AA', 'hex');
      responseCells[0].textContent = formatNumber(canIdLength === 11 ? '7E8' : '18DAF110', 'hex');
      responseCells[1].textContent = formatNumber(values.bit_length / 8 + 2, 'dec');
      responseCells[2].textContent = formatNumber('41', 'hex');
      responseCells[3].textContent = pid;
      if (values.bit_length >= 8) {
        responseInputs[0].style.display = 'block';
        responseInputs[0].nextElementSibling.style.display = 'none';
        data += dataBytes[0];
      } else {
        responseInputs[0].style.display = 'none';
        responseInputs[0].nextElementSibling.style.display = 'block';
      }
      if (values.bit_length >= 16) {
        responseInputs[1].style.display = 'block';
        responseInputs[1].nextElementSibling.style.display = 'none';
        data += dataBytes[1];
      } else {
        responseInputs[1].style.display = 'none';
        responseInputs[1].nextElementSibling.style.display = 'block';
      }
      if (values.bit_length >= 24) {
        responseInputs[2].style.display = 'block';
        responseInputs[2].nextElementSibling.style.display = 'none';
        data += dataBytes[2];
      } else {
        responseInputs[2].style.display = 'none';
        responseInputs[2].nextElementSibling.style.display = 'block';
      }
      if (values.bit_length >= 32) {
        responseInputs[3].style.display = 'block';
        responseInputs[3].nextElementSibling.style.display = 'none';
        data += dataBytes[3];
      } else {
        responseInputs[3].style.display = 'none';
        responseInputs[3].nextElementSibling.style.display = 'block';
      }
      responseCells[8].textContent = formatNumber('AA', 'hex');
      const dataDec = formatNumber(data, 'hex', 'dec');
      const resDec = Math.round((values.offset + values.scale * dataDec) * 100) / 100;
      resCalcOffset.textContent = values.offset;
      resCalcScale.textContent = values.scale_short;
      resCalcData.textContent = dataDec;
      resCalcResultDec.textContent = resDec;
      resCalcUnit.textContent = values.unit;
      if (values.name.toLowerCase().startsWith('pids supported')) {
        const resBin = `${'0'.repeat(values.bit_length)}${resDec.toString(2)}`.slice(-values.bit_length);
        resBinTable.style.display = 'block';
        resCalcResultBin.innerHTML = '';
        for (const bin of resBin) {
          const resBinElement = document.createElement('span');
          resBinElement.textContent = bin;
          resBinElement.style.color = BINARY_COLOR_MAP[bin];
          resCalcResultBin.appendChild(resBinElement);
        }
        pidsTable.style.display = 'block';
        pidsBody.innerHTML = '';
        let currentPidDec = pidDec + 1;
        for (let i = 0, n = values.bit_length; i < n; i++) {
          const currentValues = obd2PidList[currentPidDec] || {};
          const bin = resBin[i];
          const pidsRow = document.createElement('tr');
          pidsBody.appendChild(pidsRow);
          const pidsCell = document.createElement('td');
          pidsCell.textContent = formatNumber(currentPidDec, 'dec');
          pidsRow.appendChild(pidsCell);
          const pidsNameCell = document.createElement('td');
          pidsNameCell.textContent = currentValues.name;
          pidsRow.appendChild(pidsNameCell);
          const pidsSupportedCell = document.createElement('td');
          pidsSupportedCell.textContent = bin === '1' ? 'Yes' : 'No';
          pidsSupportedCell.style.color = BINARY_COLOR_MAP[bin];
          pidsRow.appendChild(pidsSupportedCell);
          currentPidDec += 1;
        }
      } else {
        resBinTable.style.display = 'none';
        pidsTable.style.display = 'none';
      }
    };
    /**
     * Takes a number or string in a `sourceFormat` and converts it to a `targetFormat`, between hexadecimal and decimal.
     *
     * @param {string | number} num
     * @param {string} currentFormat
     * @param {string} targetFormat
     */
    const formatNumber = (num, sourceFormat = oldNumberFormat, targetFormat = currentNumberFormat) => {
      if (sourceFormat === 'dec') {
        const dec = typeof num === 'string' ? strToDec(num) : num;
        return targetFormat === 'hex' ? formatHex(decToHex(dec)) : dec;
      }
      return targetFormat === 'dec' ? hexToDec(num) : formatHex(num);
    };
    /**
     * Formats a HEX string to be of at least length 2 and uppercase.
     *
     * @param {string} hex
     */
    const formatHex = (hex) => {
      return (hex.length === 1 ? `0${hex}` : hex).toUpperCase();
    };
    /**
     * Converts a decimal to a hexadecimal.
     *
     * @param {number} dec
     * @returns string
     */
    const decToHex = (dec) => {
      return (dec || 0).toString(16).toUpperCase();
    };
    /**
     * Converts a hexadecimal to a decimal.
     *
     * @param {string} hex
     * @returns number
     */
    const hexToDec = (hex) => {
      return Number.parseInt(hex || '0', 16);
    };
    /**
     * Converts a string to a decimal.
     *
     * @param {string} str
     * @returns number
     */
    const strToDec = (str) => {
      return Number.parseInt(str || '0', 10);
    };
    /**
     * INITIALIZATION
     */
    obd2PidList = await updateObd2PidList();
    updateDropdowns();
    numberFormatSelect.addEventListener('change', changeNumberFormat);
    canIdLengthSelect.addEventListener('change', updateTable);
    pidSelect.addEventListener('change', handleDropdownChange);
    nameSelect.addEventListener('change', handleDropdownChange);
    for (const responseInput of responseInputs) {
      responseInput.addEventListener('input', validateResponseInput);
      responseInput.addEventListener('change', validateResponseInput);
    }
    pidSelect.value = DEFAULT_PID;
    pidSelect.dispatchEvent(new KeyboardEvent('change'));
  })();