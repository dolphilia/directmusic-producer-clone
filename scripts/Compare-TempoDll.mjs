// Compare the same connected COM probe against original and candidate DLLs.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
if (process.argv.length !== 4) throw new Error('Usage: node scripts/Compare-TempoDll.mjs <original run> <candidate run>');
const directories = process.argv.slice(2).map(value => path.resolve(value));
const json = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
const hash = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const metadata = directories.map(directory => json(path.join(directory, 'run.json')));
for (let i = 0; i < metadata.length; ++i) {
  const run = metadata[i];
  if (run.implementation !== (i === 0 ? 'original' : 'candidate') || run.exitCode !== 0 || run.timedOut || run.launchError ||
      !run.connectedFixture) throw new Error('Successful connected runs are required');
}
if (metadata[0].originalTimeline !== metadata[1].originalTimeline) throw new Error('Timeline connection conditions must match');
if(Boolean(metadata[0].systemClipboard)!==Boolean(metadata[1].systemClipboard))throw new Error('System clipboard conditions must match');
const withTimeline = metadata[0].originalTimeline === true;
const withWindow = metadata[0].windowedTimeline === true;
if (withWindow !== (metadata[1].windowedTimeline === true)) throw new Error('Windowed Timeline conditions must match');
if (withTimeline && metadata.some(run => run.timelineModule?.sha256 !== 'bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176'))
  throw new Error('Both Timeline runs must identify the original Timeline DLL');
const withTimeSignature = Boolean(metadata[0].timeSignatureModule);
if (withTimeSignature !== Boolean(metadata[1].timeSignatureModule)) throw new Error('Time-signature connection conditions must match');
if (withTimeSignature && (!withTimeline || metadata.some(run => run.timeSignatureModule?.sha256 !== '898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7')))
  throw new Error('Both musical-position runs must identify the original TimeSigStripMgr DLL');
if (metadata[0].dllSha256 !== 'bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95' ||
    metadata[0].dllSha256 === metadata[1].dllSha256 || metadata[0].probeSha256 !== metadata[1].probeSha256)
  throw new Error('Both runs must use the same probe, the known original, and a different candidate DLL');
if (JSON.stringify(metadata[0].runtimeModules) !== JSON.stringify(metadata[1].runtimeModules))
  throw new Error('Runtime modules must match');
if (!Array.isArray(metadata[0].sources) || !metadata[0].sources.length ||
    JSON.stringify(metadata[0].sources) !== JSON.stringify(metadata[1].sources))
  throw new Error('Both runs must save identical source identities');
for (let i = 0; i < metadata.length; ++i) {
  const snapshot = path.join(directories[i], 'sources');
  for (const source of metadata[i].sources) {
    const file = path.resolve(snapshot, source.path);
    if (!file.startsWith(snapshot + path.sep) || hash(file) !== source.sha256)
      throw new Error(`Source snapshot identity mismatch: ${source.path}`);
  }
}

// Implementation-specific addresses and internal host-service call sequences
// are recorded but are not ABI behavior comparisons. Observable results remain.
const omitted = operation => operation === 'object_vtable' || operation.endsWith('_layout') || operation.startsWith('fixture_');
const logs = directories.map(directory => fs.readFileSync(path.join(directory, 'probe.jsonl'), 'utf8').trim()
  .split(/\r?\n/).map(JSON.parse).filter(record => !omitted(record.operation)));
const withPropertyPageConnection = logs.some(log => log.some(record => record.operation === 'begin_property_page_probe'));
const withNativePage = logs.some(log => log.some(record => record.operation === 'begin_native_page_probe'));
const withDrag = logs.some(log => log.some(record => record.operation === 'begin_drag_callback_probe'));
if (withWindow && metadata[0].sources.some(s => s.path === 'tests/native/drag_probe.h') && !withDrag)
  throw new Error('Windowed drag coverage is missing');
const withDragStart = logs.some(log => log.some(r => r.operation === 'begin_drag_start_case'));
const withCommands = logs.some(log => log.some(r => r.operation === 'begin_command_case'));
if(withWindow && metadata[0].sources.some(s=>s.path==='tests/native/command_probe.h') && !withCommands)
  throw new Error('Windowed command coverage is missing');
const commandCases=['delete_empty_beat','delete_occupied','delete_notification','insert_selected','insert_all_selected',
  'select_all','properties_selected','right_move_command','right_copy_command','right_cancel_command','unknown_command'];
if (withWindow && metadata[0].sources.some(s => s.path === 'tests/native/ole_drag_boundary.h') && !withDragStart)
  throw new Error('Windowed drag-start coverage is missing');
const dragStartCases = ['control_single_cancel','control_all_cancel','move_single_cancel','move_all_cancel',
  'control_all_copy','move_all_copy','control_single_move','move_all_move','control_all_failure',
  'self_single_move','self_all_copy','self_same_position','real_control_all_cancel','real_move_all_cancel'];
const dropCases = ['copy','move','control','control_without_allowed_copy','snap','origin','near_end','short_target',
  'negative','unsupported','no_effect','release_after_enter','unselected','single_selected','release_negative_over','unsupported_medium','query_sfalse',
  'right_dismiss_none','right_dismiss_copy','right_dismiss_move','right_dismiss_both','right_dismiss_link','right_dismiss_unsupported','right_dismiss_negative'];
if (withDrag && !withWindow) throw new Error('Drag cases require the windowed Timeline');
const nativeEditCases = ['tempo_normal','measure_normal','beat_normal','tick_negative','tempo_same',
  'tempo_zero','tempo_high','tempo_negative','tempo_empty','tempo_text','tempo_suffix','tempo_space',
  'measure_zero','measure_high','measure_empty','beat_high','tick_high','tick_origin'];
const pageTrackCases = ['tempo','clamp','measure','beat','tick'];
if (withNativePage && !withWindow) throw new Error('Native pages require the windowed probe');
if (withPropertyPageConnection && !withTimeSignature) throw new Error('Property-page connection probe requires the connected Timeline setup');
const boundaryCases = ['negative_tick', 'beyond_end', 'partial_measure_end', 'multiple_tempo', 'multiple_collision', 'multiple_reorder'];
const withBoundaries = logs.some(log => log.some(record => record.operation === 'begin_boundary_case'));
if (withBoundaries && !withTimeSignature) throw new Error('Boundary cases require the time-signature connection');
const meterCases = ['three_four', 'three_eight', 'five_four'];
const withMeterChanges = logs.some(log => log.some(record => record.operation === 'begin_meter_case'));
if (withMeterChanges && !withTimeSignature) throw new Error('Meter changes require the time-signature connection');
const withNotificationChecks = logs.some(log => log.some(record => record.operation === 'notification_registration_verified'));
const insertCases = ['empty', 'between', 'occupied', 'occupied_tick', 'later_measure', 'negative', 'beyond_length'];
const withInsertion = logs.some(log => log.some(record => record.operation === 'begin_insert_case'));
const copyCases = ['unselected', 'single', 'multiple', 'early_boundary'];
const withCopy = logs.some(log => log.some(record => record.operation === 'begin_copy_case'));
const pasteCases = ['merge_empty', 'snap_existing', 'merge_duplicates', 'overwrite', 'near_end', 'before_start', 'short_target'];
const withPaste = logs.some(log => log.some(record => record.operation === 'begin_paste_case'));
const tempoNotificationCases = ['empty', 'before_first', 'first', 'between', 'duplicates', 'after_last', 'unchanged', 'timeline_delivery'];
const withTempoNotification = logs.some(log => log.some(record => record.operation === 'begin_tempo_notification_case'));
const rangeCases = ['same_beat', 'end_beat', 'across_measures', 'equal', 'disabled', 'reversed', 'start_minus_one', 'cleared'];
const withRangeSelection = logs.some(log => log.some(record => record.operation === 'begin_range_case'));
const drawCases = ['empty','single','fractional','multiple','same_beat','selected','selected_same_beat','range','offset','clipped','tight','alternate_view'];
if (withWindow) drawCases.push('scroll_beat','scroll_middle','scroll_same_beat','scroll_selected',
  'ghost_single','ghost_italic','ghost_middle','ghost_selected');
const withDrawing = logs.some(log => log.some(record => record.operation === 'begin_draw_case'));
const mouseCases = ['first','second','beat_end','replace','control_add','shift_forward','shift_reverse','same_beat','empty_beat','collapse','empty_track','shift_from_empty','empty_then_select_all'];
const withMouse = logs.some(log => log.some(record => record.operation === 'begin_mouse_case'));
if (withMouse && !withWindow) throw new Error('Mouse cases require the windowed Timeline');
const withUndoLabels = logs.some(log => log.some(record => record.operation === 'undo_label'));
if (withDrawing && !withTimeSignature) throw new Error('Drawing cases require the time-signature connection');
if (withRangeSelection && !withTimeSignature) throw new Error('Range selection cases require the time-signature connection');
if (withTempoNotification && !withTimeSignature) throw new Error('Tempo notification cases require the time-signature connection');
if (withPaste && !withTimeSignature) throw new Error('Paste cases require the time-signature connection');
if (withCopy && !withTimeSignature) throw new Error('Copy cases require the time-signature connection');
if (withInsertion && !withTimeSignature) throw new Error('Insertion cases require the time-signature connection');
if (withNotificationChecks && !withTimeline) throw new Error('Notification registration checks require Timeline');
const expectedCases = ['single_137', 'fractional_93_75', 'multiple_events', 'unsorted_events', 'duplicate_times', 'replace_with_single'];
for (const log of logs) {
  if(withCommands){
    if(!withWindow || JSON.stringify(log.filter(r=>r.operation==='end_command_case'&&r.passed).map(r=>r.case))!==JSON.stringify(commandCases))
      throw new Error('Command case coverage is incomplete');
    for(const name of commandCases){
      const begin=log.findIndex(r=>r.operation==='begin_command_case'&&r.case===name);
      const end=log.findIndex(r=>r.operation==='end_command_case'&&r.case===name);
      if(begin<0||end<=begin)throw new Error(`Command boundaries are incomplete: ${name}`);
      const records=log.slice(begin,end+1);
      if(!['command_mouse_down','command_mouse_up','command_execute','command_get_selected','command_reload'].every(op=>
          records.filter(r=>r.operation===op&&r.hresult==='0x00000000').length===1) ||
          !records.some(r=>r.operation==='command_selection_present') ||
          !records.some(r=>r.operation==='undo_label'&&r.phase==='command') ||
          !records.some(r=>r.operation==='runtime_tempo'&&r.matches) ||
          (name.startsWith('delete_') ? !records.some(r=>r.operation==='command_can_copy'&&r.hresult==='0x00000001') :
           !records.some(r=>r.operation==='command_copy_selected'&&r.hresult==='0x00000000')))
        throw new Error(`Command edit/selection/runtime coverage is incomplete: ${name}`);
    }
  }
  if (withDragStart) {
    if (!withDrag || log.filter(r => r.operation === 'drag_start_hook' && r.installed).length !== 1 ||
        log.filter(r => r.operation === 'drag_start_hook_restored' && r.restored).length !== 1 ||
        JSON.stringify(log.filter(r => r.operation === 'end_drag_start_case' && r.passed && r.calls === 1).map(r => r.case)) !== JSON.stringify(dragStartCases))
      throw new Error('Drag-start boundary/lifetime coverage is incomplete');
    const phases = [0,1].flatMap(escape => [0,1,2,3,4,8,9,10,16,256].map(keys => [escape,keys]));
    for (const name of dragStartCases) {
      const begin = log.findIndex(r => r.operation === 'begin_drag_start_case' && r.case === name);
      const end = log.findIndex(r => r.operation === 'end_drag_start_case' && r.case === name);
      if (begin < 0 || end <= begin) throw new Error('Drag-start case boundaries are incomplete');
      const records = log.slice(begin,end+1);
      if (!['drag_start_identity','drag_start_retained_data'].every(op => records.some(r => r.operation === op && r.same)) ||
          !records.some(r => r.operation === 'drag_start_retained_data' && r.released) ||
          !records.some(r => r.operation === 'drag_start_boundary' && r.allowed === 3) ||
          !records.some(r => r.operation === 'drag_start_data' && r.read) ||
          !records.some(r => r.operation === 'drag_start_shared_stream' && !r.same && r.position > 0) ||
          !records.some(r => r.operation === 'drag_start_format' && r.hresult === '0x00000000' && r.count === 1 && r.tempo && r.medium === 4 && r.aspect === 1 && r.index === -1) ||
          !records.some(r => r.operation === 'drag_start_enum_end' && r.hresult === '0x00000001' && r.count === 0) ||
          !records.some(r => r.operation === 'drag_start_up' && r.hresult === '0x00000000') ||
          JSON.stringify(records.filter(r => r.operation === 'drag_start_selection_present').map(r => r.phase)) !== JSON.stringify([0,1]) ||
          !records.some(r => r.operation === 'drag_start_second_move' && r.hresult === '0x00000000') ||
          JSON.stringify(records.filter(r => r.operation === 'drag_start_continue').map(r => [r.escape,r.keys])) !== JSON.stringify(phases))
        throw new Error(`Drag-start data/continuation coverage is incomplete: ${name}`);
      if (name.startsWith('self_') && (!records.some(r => r.operation === 'drag_start_self_drop' && r.hresult === '0x00000000') ||
          !records.some(r => r.operation === 'drag_start_drop_runtime' && r.hresult === '0x00000000')))
        throw new Error(`Self-drop/runtime coverage is incomplete: ${name}`);
      if (name.startsWith('real_') && (!records.some(r => r.operation === 'drag_start_real_wake' && r.owned_thread && r.posted) ||
          !records.some(r => r.operation === 'drag_start_real_continue' && r.forced_escape && r.hresult === '0x00040101') ||
          !records.some(r => r.operation === 'drag_start_real_loop' && r.hresult === '0x00040101' && r.effect === 0 && r.continuations > 0 && r.source_refs === 1)))
        throw new Error(`Real OLE cancellation coverage is incomplete: ${name}`);
    }
  }
  if (withDrag) {
    if (log.filter(r => r.operation === 'begin_drag_callback_probe').length !== 1 ||
        log.filter(r => r.operation === 'end_drag_callback_probe' && r.passed).length !== 1 ||
        !['drag_initialize_strip','drag_query_source','drag_query_target'].every(op =>
          log.filter(r => r.operation === op && r.hresult === '0x00000000').length === 1) ||
        log.filter(r => r.operation === 'drag_identity' && r.same).length !== 2)
      throw new Error('Drag interface/lifetime coverage is incomplete');
    const continuation = [0,1].flatMap(escape => [0,1,2,3,4,8,9,10,16,256].map(keys => [escape,keys]));
    if (JSON.stringify(log.filter(r => r.operation === 'drag_continue_initial').map(r => [r.escape,r.keys])) !== JSON.stringify(continuation) ||
        JSON.stringify(log.filter(r => r.operation === 'drag_feedback' && r.hresult === '0x00040102').map(r => r.effect)) !== JSON.stringify([0,1,2,3,4]))
      throw new Error('Drag continuation/feedback coverage is incomplete');
    const entered = [4,1].flatMap(medium => [true,false].flatMap(valid => [192,-1].flatMap(x => [1,9,2].flatMap(keys => [0,1,2,3,4].map(allowed => [medium,valid,x,keys,allowed])))));
    if (JSON.stringify(log.filter(r => r.operation === 'drag_enter' && r.hresult === '0x00000000' && r.refs === 2).map(r => [r.medium,r.valid,r.x,r.keys,r.allowed])) !== JSON.stringify(entered) ||
        log.filter(r => r.operation === 'drag_over' && r.hresult === '0x00000000' && r.refs === 2).length !== entered.length ||
        log.filter(r => r.operation === 'drag_leave' && r.hresult === '0x00000000' && r.refs === 1).length !== entered.length)
      throw new Error('Drag transfer/ownership coverage is incomplete');
    if (JSON.stringify(log.filter(r => r.operation === 'end_drop_case' && r.passed).map(r => r.case)) !== JSON.stringify(dropCases) ||
        log.filter(r => r.operation === 'drop_execute' && r.hresult === '0x00000000' && r.refs === 1).length !== dropCases.length ||
        log.filter(r => r.operation === 'drop_cursor_mode' && r.cursor === 100 && r.mode === 1).length !== dropCases.length ||
        log.filter(r => r.operation === 'drop_copy_selection' && r.hresult === '0x00000000').length !== dropCases.length ||
        log.filter(r => r.operation === 'drop_reload' && r.hresult === '0x00000000').length !== dropCases.length)
      throw new Error('Drop editing/persistence coverage is incomplete');
    if (metadata[0].sources.some(s => s.path === 'tests/native/drop_menu_boundary.h') &&
        (log.filter(r => r.operation === 'drop_menu_hook' && r.installed).length !== 1 ||
         log.filter(r => r.operation === 'drop_menu_hook_restored' && r.restored).length !== 1 ||
         log.filter(r => r.operation === 'drop_menu_popup' && r.flags === 2 && r.owned && r.hidden && r.count === 4).length !== 7 ||
         log.filter(r => r.operation === 'drop_menu_item').length !== 28 ||
         log.filter(r => r.operation === 'drop_menu_dismissed' && r.observed && r.effect === 0).length !== 7))
      throw new Error('Right-drop menu/dismissal coverage is incomplete');
    for (const name of dropCases.filter(name => name.startsWith('right_'))) {
      const begin = log.findIndex(r => r.operation === 'begin_drop_case' && r.case === name);
      const end = log.findIndex(r => r.operation === 'end_drop_case' && r.case === name);
      if (begin < 0 || end <= begin) throw new Error(`Right-drop boundaries are incomplete: ${name}`);
      const records = log.slice(begin,end+1);
      const gray = ['right_dismiss_move','right_dismiss_both'].includes(name) ? 0 : 1;
      const items = records.filter(r => r.operation === 'drop_menu_item').map(r => [r.index,r.id,r.state,r.label]);
      if (JSON.stringify(items) !== JSON.stringify([[0,0x8026,gray,'Move'],[1,0x8028,0,'Copy'],[2,0,2051,''],[3,0x8027,0,'Cancel']]) ||
          !records.some(r => r.operation === 'drop_menu_dismissed' && r.observed && r.effect === 0))
        throw new Error(`Right-drop menu conditions are incomplete: ${name}`);
    }
  }
  if (withNativePage) {
    if (log.filter(r => r.operation === 'begin_native_page_probe').length !== 1 ||
        !log.some(r => r.operation === 'native_page_create' && r.hresult === '0x00000000' && r.count === 1 && r.handle) ||
        !log.some(r => r.operation === 'native_page_window' && r.valid && r.hidden && r.child) ||
        !log.some(r => r.operation === 'native_page_cancel' && r.unchanged && r.result === 0) ||
        !log.some(r => r.operation === 'end_native_page_probe' && r.passed && r.object_refs === 1 && r.removals === 1))
      throw new Error('Native property-page creation/lifetime coverage is incomplete');
    const edits = log.filter(r => r.operation === 'native_page_edit').map(r => r.case);
    if (JSON.stringify(edits) !== JSON.stringify(nativeEditCases)) throw new Error('Native property-page edit coverage is incomplete');
    const states = log.filter(r => r.operation === 'native_page_state').map(r => r.case);
    if (JSON.stringify(states) !== JSON.stringify(['initial','single','multiple','none',...nativeEditCases]) ||
        log.filter(r => r.operation === 'native_page_control').length !== states.length * 8)
      throw new Error('Native property-page state coverage is incomplete');
    const spins = log.filter(r => r.operation === 'native_page_spin').map(r => [r.id,r.delta]);
    if (JSON.stringify(spins) !== JSON.stringify([[232,-1],[232,1],[229,1],[230,-1],[234,1]]))
      throw new Error('Native property-page spin coverage is incomplete');
    const trackCases = log.filter(r => r.operation === 'end_page_track_case' && r.passed).map(r => r.case);
    if (JSON.stringify(trackCases) !== JSON.stringify(pageTrackCases) ||
        !log.some(r => r.operation === 'page_track_remove_object' && r.hresult === '0x00000000'))
      throw new Error('Native property-page track editing/persistence coverage is incomplete');
  }
  if (withPropertyPageConnection) {
    if (log.filter(record => record.operation === 'begin_property_page_probe').length !== 1 ||
        log.filter(record => record.operation === 'end_property_page_probe' && record.passed).length !== 1 ||
        !log.some(record => record.operation === 'page_title_value' && record.tempo && record.append_properties === 1) ||
        !log.some(record => record.operation === 'page_object_lifecycle' && record.first_refs === 1 && record.second_refs === 1 &&
            record.first_gets === 1 && record.second_gets === 2 && record.first_removals === 1 && record.second_removals === 1) ||
        !log.some(record => record.operation === 'page_sheet_lifecycle' && record.framework_refs === 0 && record.sheet_refs === 1 &&
            record.set_calls === 4 && record.visible_calls === 5))
      throw new Error('Property-page connection/lifetime coverage is incomplete');
  }
  const ended = log.filter(record => record.operation === 'end_stream_case' && record.passed).map(record => record.case);
  if (JSON.stringify(ended) !== JSON.stringify(expectedCases)) throw new Error('Reference case coverage is incomplete');
  if (!log.some(record => record.operation === 'can_unload_after_release' && record.hresult === '0x00000000'))
    throw new Error('Missing successful unload observation');
  if (withTimeline && (!log.some(record => record.operation === 'property_edit_verified' && record.passed) ||
      !log.some(record => record.operation === 'delete_observation' && record.empty) ||
      !log.some(record => record.operation === 'timeline_can_unload_after_release' && record.hresult === '0x00000000')))
    throw new Error('Timeline editing or unload observations are incomplete');
  if (withTimeSignature) {
    const positions = log.filter(record => record.operation === 'end_position_case' && record.passed).map(record => record.time);
    if (JSON.stringify(positions) !== JSON.stringify([768, 900, 4708]) ||
        !log.some(record => record.operation === 'timeline_convert_position' && record.hresult === '0x00000000') ||
        !log.some(record => record.operation === 'time_signature_can_unload_after_release' && record.hresult === '0x00000000'))
      throw new Error('Musical-position editing or time-signature unload observations are incomplete');
  }
  if (withBoundaries) {
    const endedBoundaries = log.filter(record => record.operation === 'end_boundary_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedBoundaries) !== JSON.stringify(boundaryCases)) throw new Error('Boundary case coverage is incomplete');
  }
  if (withMeterChanges) {
    const endedMeters = log.filter(record => record.operation === 'end_meter_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedMeters) !== JSON.stringify(meterCases)) throw new Error('Meter-change case coverage is incomplete');
  }
  if (withNotificationChecks) {
    const phases = log.filter(record => record.operation === 'notification_registration_verified' && record.passed).map(record => record.connected);
    if (JSON.stringify(phases) !== JSON.stringify([true, false])) throw new Error('Notification registration checks are incomplete');
  }
  if (withInsertion) {
    const endedInsertion = log.filter(record => record.operation === 'end_insert_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedInsertion) !== JSON.stringify(insertCases)) throw new Error('Insertion case coverage is incomplete');
  }
  if (withCopy) {
    const endedCopy = log.filter(record => record.operation === 'end_copy_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedCopy) !== JSON.stringify(copyCases)) throw new Error('Copy case coverage is incomplete');
  }
  if (withUndoLabels) {
    const expected = { initial: 1, ...(withTimeline ? {tempo_change: 1, delete: 1} : {}),
      ...(withTimeSignature ? {move: 3} : {}), ...(withInsertion ? {insert: 7} : {}), ...(withCopy ? {copy: 4, cut: 4} : {}),
      ...(withPaste ? {paste: pasteCases.length} : {}),
      ...(withTempoNotification ? {before_tempo_notification: tempoNotificationCases.length, after_tempo_notification: tempoNotificationCases.length} : {}) };
    for (const [phase, count] of Object.entries(expected)) {
      if (log.filter(record => record.operation === 'undo_label' && record.phase === phase &&
          record.supported === '0x00000000' && record.hresult === '0x00000000').length !== count)
        throw new Error(`Undo label coverage is incomplete: ${phase}`);
    }
  }
  if (withPaste) {
    const endedPaste = log.filter(record => record.operation === 'end_paste_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedPaste) !== JSON.stringify(pasteCases)) throw new Error('Paste case coverage is incomplete');
    for (const operation of ['paste_connect_time_strip', 'paste_disconnect_time_strip']) {
      if (log.filter(record => record.operation === operation && record.hresult === '0x00000000').length !== 1)
        throw new Error(`Time-strip lifecycle observation is incomplete: ${operation}`);
    }
    const cursors = log.filter(record => record.operation === 'paste_cursor');
    if (cursors.length !== pasteCases.length || cursors.some(record => record.actual !== record.requested) ||
        JSON.stringify(cursors.map(record => record.actual)) !== JSON.stringify([900, 900, 0, 0, 30000, 0, 900]))
      throw new Error('Paste cursor readback is incomplete or incorrect');
    for (const [operation, hresult] of [['paste_format_after_inspection', '0x00000001'], ['paste_format_before_execution', '0x00000000']]) {
      if (log.filter(record => record.operation === operation && record.hresult === hresult).length !== pasteCases.length)
        throw new Error(`Paste data availability observation is incomplete: ${operation}`);
    }
  }
  if (withTempoNotification) {
    const endedNotifications = log.filter(record => record.operation === 'end_tempo_notification_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedNotifications) !== JSON.stringify(tempoNotificationCases)) throw new Error('Tempo notification case coverage is incomplete');
    for (const operation of ['tempo_notification_connect_time_strip', 'tempo_notification_disconnect_time_strip']) {
      if (log.filter(record => record.operation === operation && record.hresult === '0x00000000').length !== 1)
        throw new Error(`Tempo notification time-strip lifecycle observation is incomplete: ${operation}`);
    }
    const cursors = log.filter(record => record.operation === 'tempo_notification_cursor');
    if (cursors.length !== tempoNotificationCases.length || cursors.some(record => record.actual !== record.requested) ||
        JSON.stringify(cursors.map(record => record.actual)) !== JSON.stringify([0, 0, 0, 1000, 900, 30000, 900, 900]))
      throw new Error('Tempo notification cursor readback is incomplete or incorrect');
    if (!log.some(record => record.operation === 'tempo_notification_null' && record.hresult === '0x80004003'))
      throw new Error('Tempo notification null argument observation is incomplete');
    const properties = log.filter(record => record.operation === 'tempo_notification_property12');
    if (properties.some(record => record.type !== 11) ||
        JSON.stringify(properties.map(record => record.value)) !== JSON.stringify([0, 0, 0, 0, 0, 1, 1, 1]))
      throw new Error('Tempo notification property restoration is incomplete or incorrect');
  }
  if (withRangeSelection) {
    const endedRanges = log.filter(record => record.operation === 'end_range_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedRanges) !== JSON.stringify(rangeCases)) throw new Error('Range selection case coverage is incomplete');
    for (const operation of ['range_wrong_type', 'range_unknown_property']) {
      if (!log.some(record => record.operation === operation && record.hresult === '0x80004005'))
        throw new Error(`Range selection property validation is incomplete: ${operation}`);
    }
    if (!log.some(record => record.operation === 'range_reset_gutter' && record.hresult === '0x00000000'))
      throw new Error('Range selection cleanup is incomplete');
  }
  if (withDrawing) {
    if (withWindow) {
      if (!log.some(record => record.operation === 'windowed_probe_dep' && record.enabled && record.flags === 1 && record.permanent))
        throw new Error('Windowed probe requires verified DEP policy');
      for (const operation of ['draw_activate_window', 'draw_close_window', 'draw_restore_scroll'])
        if (!log.some(record => record.operation === operation && record.hresult === '0x00000000'))
          throw new Error(`Missing window lifecycle operation: ${operation}`);
      const scrolls = log.filter(record => record.operation === 'draw_scroll_readback');
      if (scrolls.length !== drawCases.length || scrolls.some(record => !record.exact || record.actual !== record.requested))
        throw new Error('Scroll positions were not read back exactly');
      const starts = log.filter(record => record.operation === 'draw_visible_start').map(record => record.time);
      if (JSON.stringify(starts.slice(-8)) !== JSON.stringify([1536,1600,1536,1536,1536,1536,1600,1536]))
        throw new Error('Nonzero visible starts were not exercised');
    }
    const endedDraws = log.filter(record => record.operation === 'end_draw_case' && record.passed).map(record => record.case);
    if (JSON.stringify(endedDraws) !== JSON.stringify(drawCases)) throw new Error('Drawing case coverage is incomplete');
    if (log.filter(record => record.operation === 'draw_strip' && record.hresult === '0x00000000').length !== drawCases.length * 2)
      throw new Error('Drawing calls or repeat observations are incomplete');
    const states = log.filter(record => record.operation === 'draw_dc_state');
    if (states.length !== drawCases.length * 2 || states.some(record => !record.font_preserved || record.text_color !== 0 ||
        record.background_color !== 0xffffff || record.background_mode !== 1))
      throw new Error('Drawing device-context state is not preserved');
    if (!log.some(record => record.operation === 'draw_font' && record.bytes > 0)) throw new Error('Missing drawing font evidence');
  }
  if (withMouse) {
    const ended=log.filter(record=>record.operation==='end_mouse_case'&&record.passed).map(record=>record.case);
    if(JSON.stringify(ended)!==JSON.stringify(mouseCases)) throw new Error('Mouse selection coverage is incomplete');
    if(log.filter(record=>record.operation==='mouse_dispatch'&&record.hresult==='0x00000000').length!==36)
      throw new Error('Mouse down/up observations are incomplete');
    if(log.filter(record=>record.operation==='mouse_draw'&&record.hresult==='0x00000000').length!==mouseCases.length*2)
      throw new Error('Mouse selection drawings are incomplete');
    const saved=log.filter(record=>record.operation==='mouse_saved_unchanged');
    if(saved.length!==mouseCases.length||saved.some(record=>!record.same)) throw new Error('Selection changed saved event data');
  }
}
const differences = [];
for (let i = 0; i < Math.max(logs[0].length, logs[1].length); ++i) {
  if (JSON.stringify(logs[0][i]) !== JSON.stringify(logs[1][i])) differences.push({ index: i, original: logs[0][i], candidate: logs[1][i] });
}
const files = ['initial-stream.bin', ...['single', 'fractional', 'multiple', 'unsorted', 'duplicate', 'replace']
  .flatMap(name => [`${name}-input.bin`, `${name}-output.bin`])];
if (withTimeline) files.push('property-edit-output.bin', 'delete-output.bin');
if (withTimeSignature) files.push('timesig-input.bin', ...[768, 900, 4708].flatMap(time =>
  [`position-${time}-input.bin`, `position-${time}-tempo-output.bin`, `position-${time}-move-output.bin`]));
if (withBoundaries) files.push(...boundaryCases.flatMap(name =>
  [`boundary-${name}-input.bin`, `boundary-${name}-output.bin`, `boundary-${name}-reload-output.bin`]));
if (withMeterChanges) files.push(...meterCases.flatMap(name =>
  [`meter-${name}-input.bin`, `meter-${name}-time-signature-input.bin`, `meter-${name}-output.bin`, `meter-${name}-refresh-output.bin`]));
if (withInsertion) files.push(...insertCases.flatMap(name =>
  [`insert-${name}-input.bin`, `insert-${name}-output.bin`, `insert-${name}-reload-output.bin`]));
if (withCopy) files.push(...copyCases.flatMap(name =>
  [`copy-${name}-input.bin`, `copy-${name}-output.bin`, `copy-${name}-cut-output.bin`]));
if (withPaste) files.push(...pasteCases.flatMap(name =>
  [`paste-${name}-source.bin`, `paste-${name}-target.bin`, `paste-${name}-output.bin`, `paste-${name}-reload-output.bin`]));
if (withTempoNotification) files.push(...tempoNotificationCases.flatMap(name =>
  [`tempo-notification-${name}-input.bin`, `tempo-notification-${name}-output.bin`, `tempo-notification-${name}-reload-output.bin`]));
if (withRangeSelection) files.push(...rangeCases.flatMap(name =>
  [`range-${name}-input.bin`, `range-${name}-selected-output.bin`, `range-${name}-output.bin`, `range-${name}-reload-output.bin`]));
if (withDrawing) files.push('draw-font.bin',...drawCases.flatMap(name => [`draw-${name}-input.bin`,`draw-${name}-after.bin`]));
if (withMouse) files.push(...mouseCases.flatMap(name=>[`mouse-${name}-input.bin`,`mouse-${name}-after.bin`]));
if (withMouse) files.push(...['empty_beat','shift_from_empty'].flatMap(name=>[`mouse-${name}-edited.bin`,`mouse-${name}-reload.bin`]));
if (withNativePage) files.push(...pageTrackCases.flatMap(name =>
  [`page-track-${name}-input.bin`,`page-track-${name}-output.bin`,`page-track-${name}-reload.bin`]));
if (withDrag) files.push(...dropCases.flatMap(name => ['source','target','output','reload'].map(suffix => `drop-${name}-${suffix}.bin`)));
if (withDragStart) files.push(...dragStartCases.flatMap(name => ['input','returned','up','reload'].map(suffix => `drag-start-${name}-${suffix}.bin`)));
if(withCommands)files.push(...commandCases.flatMap(name=>['input','output','reload'].map(suffix=>`command-${name}-${suffix}.bin`)));
const byteChecks = files.map(file => {
  const hashes = directories.map(directory => hash(path.join(directory, file)));
  return { file, originalSha256: hashes[0], candidateSha256: hashes[1], same: hashes[0] === hashes[1] };
});
// Original TempoStripMgr RVA 0x6362..0x6383 writes only time, double BPM,
// and tick in a 24-byte stack record. +4..7 and +20..23 are unwritten padding.
// Preserve raw hashes and compare every other byte, including the chunk header.
function normalizeCopy(bytes) {
  const out = Buffer.from(bytes);
  if (!out.length) return out;
  if (out.length < 12 || out.toString('ascii', 0, 4) !== 'tetr' || out.readUInt32LE(4) !== out.length - 8 ||
      out.readUInt32LE(8) !== 24 || (out.length - 12) % 24) throw new Error('Unexpected copy format; cannot normalize');
  for (let offset = 12; offset < out.length; offset += 24) {
    out.fill(0, offset + 4, offset + 8);
    out.fill(0, offset + 20, offset + 24);
  }
  return out;
}
const copyFiles = withCopy ? copyCases.flatMap(name => [`copy-${name}-clipboard.bin`, `copy-${name}-cut-clipboard.bin`]) : [];
if (withDrag) copyFiles.push(...dropCases.flatMap(name => [`drop-${name}-clipboard.bin`,`drop-${name}-selected-clipboard.bin`]));
if (withDragStart) copyFiles.push(...dragStartCases.flatMap(name => [`drag-start-${name}-clipboard.bin`,`drag-start-${name}-selected-clipboard.bin`]));
if(withCommands)copyFiles.push(...commandCases.map(name=>`command-${name}-selected-clipboard.bin`));
if (withPaste) copyFiles.push(...pasteCases.map(name => `paste-${name}-clipboard.bin`));
if (withRangeSelection) copyFiles.push(...rangeCases.map(name => `range-${name}-clipboard.bin`));
if (withMouse) copyFiles.push(...mouseCases.map(name=>`mouse-${name}-copy.bin`));
const copyChecks = copyFiles.map(file => {
  const bytes = directories.map(directory => fs.readFileSync(path.join(directory, file)));
  return { file, originalSha256: hash(path.join(directories[0], file)), candidateSha256: hash(path.join(directories[1], file)),
    sameBytes: bytes[0].equals(bytes[1]), sameContent: normalizeCopy(bytes[0]).equals(normalizeCopy(bytes[1])),
    normalization: '24-byte tetr record padding +4..7 and +20..23 only; raw files retained' };
});
const imageFiles = withDrawing ? drawCases.flatMap(name => [`draw-${name}-output.bmp`,`draw-${name}-repeat.bmp`]) : [];
if(withMouse)imageFiles.push(...mouseCases.flatMap(name=>[`mouse-${name}-output.bmp`,`mouse-${name}-repeat.bmp`]));
const imageChecks = imageFiles.map(file => {
  const bytes = directories.map(directory => fs.readFileSync(path.join(directory,file)));
  for (const bitmap of bytes) {
    if (bitmap.length !== 54 + 640 * 20 * 4 || bitmap.toString('ascii',0,2) !== 'BM' || bitmap.readUInt32LE(10) !== 54 ||
        bitmap.readUInt32LE(14) !== 40 || bitmap.readInt32LE(18) !== 640 || bitmap.readInt32LE(22) !== -20 ||
        bitmap.readUInt16LE(28) !== 32 || bitmap.readUInt32LE(30) !== 0) throw new Error('Unexpected drawing bitmap format');
  }
  let differentPixels = 0;
  const grayPixels = bytes.map(bitmap => {
    let count = 0;
    for (let offset = 54; offset < bitmap.length; offset += 4)
      if ((bitmap.readUInt32LE(offset) & 0xffffff) === 0xa8a8a8) ++count;
    return count;
  });
  const ghostPresent = !file.startsWith('draw-ghost_') || grayPixels.every(count => count > 0);
  for (let offset = 54; offset < bytes[0].length; offset += 4) if (bytes[0].readUInt32LE(offset) !== bytes[1].readUInt32LE(offset)) ++differentPixels;
  return {file,originalSha256:hash(path.join(directories[0],file)),candidateSha256:hash(path.join(directories[1],file)),
    same:bytes[0].equals(bytes[1]),differentPixels,grayPixels,ghostPresent};
});
const report = {
  createdUtc: new Date().toISOString(), original: directories[0], candidate: directories[1], withTimeline, withTimeSignature, withBoundaries, withMeterChanges, withNotificationChecks, withInsertion,
  withCopy, withPaste, withTempoNotification, withRangeSelection, withDrawing, withWindow, withMouse, withUndoLabels, withPropertyPageConnection, withNativePage, withCommands,
  passed: differences.length === 0 && byteChecks.every(check => check.same) && copyChecks.every(check => check.sameContent) && imageChecks.every(check => check.same && check.ghostPresent),
  comparedRecords: logs[0].length, differences, byteChecks, copyChecks, imageChecks,
  evidence: directories.map(directory => ({ directory, metadataSha256: hash(path.join(directory, 'run.json')), logSha256: hash(path.join(directory, 'probe.jsonl')) })),
  comparatorSha256: hash(fileURLToPath(import.meta.url)),
};
const output = path.join(root, 'work/comparison/tempo-dll', new Date().toISOString().replace(/[-:.]/g, ''));
fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, 'comparison.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ passed: report.passed, comparedRecords: report.comparedRecords, byteChecks: byteChecks.length,
  copyChecks: copyChecks.length, imageChecks: imageChecks.length,
  differences: differences.length, evidence: output }, null, 2));
if (!report.passed) process.exitCode = 1;
