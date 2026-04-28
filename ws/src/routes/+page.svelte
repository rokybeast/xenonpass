<script lang="ts">
	import { listEntries, createEntry, deleteEntry, type Entry } from '$lib/api';

	let vault_path = '';
	let master_password = '';
	let loggedIn = false;
	let entries: Entry[] = [];
	let error = '';

	let newEntry = {
		name: '',
		username: '',
		password: '',
		url: '',
		notes: ''
	};

	async function login() {
		try {
			const res = await listEntries({ vault_path, master_password });
			entries = res.entries;
			loggedIn = true;
			error = '';
		} catch (err: any) {
			error = 'Failed to open vault: ' + err.message;
		}
	}

	function logout() {
		vault_path = '';
		master_password = '';
		loggedIn = false;
		entries = [];
		error = '';
	}

	async function handleAdd() {
		try {
			await createEntry({
				vault_path,
				master_password,
				...newEntry
			});
			newEntry = { name: '', username: '', password: '', url: '', notes: '' };
			await login();
		} catch (err: any) {
			error = 'Failed to add entry: ' + err.message;
		}
	}

	async function handleDelete(id: string) {
		try {
			await deleteEntry({ vault_path, master_password, id });
			await login();
		} catch (err: any) {
			error = 'Failed to delete entry: ' + err.message;
		}
	}
</script>

<h1>xenonpass Web Vault</h1>

{#if error}
	<p style="color: red;"><strong>Error:</strong> {error}</p>
{/if}

{#if !loggedIn}
	<h2>Open Vault</h2>
	<form on:submit|preventDefault={login}>
		<div>
			<label for="path">Vault Path:</label>
			<input
				type="text"
				id="path"
				bind:value={vault_path}
				required
				placeholder="e.g., my_vault.xpass"
			/>
		</div>
		<div>
			<label for="password">Master Password:</label>
			<input type="password" id="password" bind:value={master_password} required />
		</div>
		<button type="submit">Unlock / Create</button>
	</form>
{:else}
	<div>
		<p>Vault: <strong>{vault_path}</strong></p>
		<button on:click={logout}>Lock Vault</button>
	</div>

	<hr />

	<h2>Add Entry</h2>
	<form on:submit|preventDefault={handleAdd}>
		<div>
			<label>Name: <input type="text" bind:value={newEntry.name} required /></label>
		</div>
		<div>
			<label>Username: <input type="text" bind:value={newEntry.username} /></label>
		</div>
		<div>
			<label>Password: <input type="text" bind:value={newEntry.password} /></label>
		</div>
		<div>
			<label>URL: <input type="text" bind:value={newEntry.url} /></label>
		</div>
		<div>
			<label>Notes: <textarea bind:value={newEntry.notes}></textarea></label>
		</div>
		<button type="submit">Add Entry</button>
	</form>

	<hr />

	<h2>Entries ({entries.length})</h2>
	{#if entries.length === 0}
		<p>No entries found.</p>
	{:else}
		<ul>
			{#each entries as entry}
				<li>
					<h3>{entry.name}</h3>
					<p>Username: {entry.username}</p>
					<p>Password: {entry.password}</p>
					<p>URL: <a href={entry.url} target="_blank">{entry.url}</a></p>
					<p>Notes: {entry.notes}</p>
					<button on:click={() => handleDelete(entry.id)}>Delete</button>
				</li>
			{/each}
		</ul>
	{/if}
{/if}
