<template>
  <section class="container mt-5">
    <nav class="navbar navbar-dark bg-secondary mb-3">
      <span class="navbar-brand mb-0 h1">連絡先リスト</span>
      <span v-if='account' class="navbar-text">{{account.name}}@{{account.authority}}</span>
    </nav>
    <b-alert :show='this.warning' variant="warning" dismissible>{{this.warning}}</b-alert>
    <b-alert :show='this.info' variant="success" dismissible>{{this.info}}</b-alert>
    <table class="table table-striped">
      <thead>
        <tr>
          <th>ID</th>
          <th>名前</th>
          <th>住所</th>
          <th>電話番号</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        <tr v-for='contact in contacts' :key='contact.id'>
          <td>{{contact.id}}</td>
          <td>{{contact.name}}</td>
          <td>{{contact.address}}</td>
          <td>{{contact.tel}}</td>
          <td>
            <b-btn variant="outline-success" size='sm' @click="editContact(contact)">編集</b-btn>
            <b-btn variant="outline-danger" size='sm' @click="deleteContact(contact)">削除</b-btn>
          </td>
        </tr>
      </tbody>
    </table>
    <b-btn variant="outline-primary" @click="newContact">新規登録</b-btn>

    <b-modal v-model="showModal" id="contactModal" :title="modalTitle" centered>
      <b-form>
        <b-form-group label=" 名前" label-for="name">
          <b-form-input type="text" v-model="modalContact.name" :state="!$v.modalContact.name.$invalid" aria-describedby="nameFeedback"></b-form-input>
          <b-form-invalid-feedback id="nameFeedback">名前は 2-10 文字で入力してください</b-form-invalid-feedback>
        </b-form-group>
        <b-form-group label=" 住所" label-for="address">
          <b-form-input type="text" v-model="modalContact.address" :state="!$v.modalContact.address.$invalid" aria-describedby="addressFeedback"></b-form-input> <b-form-invalid-feedback id="addressFeedback">住所は 4-20 文字で入力してください</b-form-invalid-feedback>
        </b-form-group>
        <b-form-group label=" 電話番号" label-for="tel">
          <b-form-input type="text" v-model="modalContact.tel" :state="!$v.modalContact.tel.$invalid" aria-describedby="telFeedback"></b-form-input>
          <b-form-invalid-feedback id="telFeedback">電話番号は 10-13 文字で入力してください</b-form-invalid-feedback>
        </b-form-group>
      </b-form>

      <div slot="modal-footer" class="w-100">
        <div class="float-right">
          <b-btn size="sm" variant="default" @click="showModal = false">キャンセル</b-btn>
          <b-btn size="sm" class="ml-1" :variant="modalButtonClass" @click="saveContact" :disabled="$v.modalContact.$invalid">
            {{modalButtonText}}
          </b-btn>
        </div>
      </div>
    </b-modal>
  </section>
</template>

<script>
import Vue from 'vue'
import Eos from 'eosjs'
import ScatterJS from 'scatter-js/dist/scatter.esm'
import { validationMixin } from "vuelidate"
import { required, minLength, maxLength }from "vuelidate/lib/validators"

const CONTRACT_ACCOUNT = 'eosbookaddrs'
export default {
  data () {
    return {
      eos: null,
      account: null,
      eosContract: null,
      info: null,
      warning: null,
      modalContact: {},
      showModal: false,
      contacts: [],
    }
  }, mixins: [
    validationMixin
  ],
  computed: {
    idExists () {
      return this.modalContact && this.modalContact.id
    },
    modalTitle () {
      return this.idExists ? ' 連絡先編集' : ' 連絡先新規登録'
    },
    modalButtonClass () {
      return 'outline-' + (this.idExists ? 'success' : 'primary')
    },
    modalButtonText () {
      return this.idExists ? ' 更新する' : ' 登録する'
    },
  },
  validations: {
    modalContact: {
      name: {
        required,
        minLength: minLength(2),
        maxLength: maxLength(10)
      },
      address: {
        required,
        minLength: minLength(4),
        maxLength: maxLength(20) },
      tel: {
        required,
        minLength: minLength(10),
        maxLength: maxLength(13)
      },
    }
  },
  async asyncData (context) {
    const network = {
      blockchain: 'eos',
      protocol: 'http',
      host: '0.0.0.0',
      port: 7777,
      chainId: 'cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f',
    }

    const connected = await ScatterJS.scatter.connect("EOS Addressbook")
    if(!connected) return false;

    const scatter = ScatterJS.scatter;
    const requiredFields = { accounts: [network] };

    await scatter.getIdentity(requiredFields);
    const account = scatter.identity.accounts.find(x => x.blockchain === 'eos');
    const eosOptions = { expireInSeconds: 60 };
    const eos = scatter.eos(network, Eos, eosOptions);
    window.scatter = null;

    const result = await eos.getTableRows(true, 'addressbook', 'bob', 'people')

    return { eos, account, contacts: result.rows }
  },
  asyncrefreshContracts(){
    const result = await this.eos.getTableRows(true, CONTRACT_ACCOUNT, CONTRACT_ACCOUNT,'people')
    this.contacts = result.rows
  },
  methods: {
    newContact () {
      this.modalContact = {}
      this.showModal = true
    },
    editContact (contact) {
      this.modalContact = Object.assign({}, contact)
      this.showModal = true
    },
    deleteContact (contact) {
      if (confirm(' 連絡先を削除します。よろしいですか?')){
        this.warning = null
        this.info = null

        this.eosContract.destroy(
          this.account.name, contact.id,
          { authorization: this.account.name }
        ).then(result => {
          this.warning = ' トランザクションを送信しました'
        }).then(async () => {
          this.info = ' 連絡先を削除しました'
        })
      }
    },
    saveContact () {
      if (this.idExists){
        this.updateContact()
      } else {
        this.createContact()
      }
      this.showModal = false
    },
    createContact () {
      this.warning = null
      this.info = null

      this.eosContract.create(
        this.account.name, this.modalContact.name, this.modalContact.address, this.modalContact.tel,
        { authorization: this.account.name }
      ).then(result => {
        this.warning = ' トランザクションを送信しました'
      }).then(async () => {
        this.info = ' 連絡先を登録しました'
      })
    },
    updateContact (contact) {
      this.warning = null
      this.info = null

      this.eosContract.update(
        this.account.name, this.modalContact.id, this.modalContact.name, this.modalContact.address,this.modalContact.tel,
        { authorization: this.account.name }
      ).then(result => {
        this.warning = ' トランザクションを送信しました' }).then(async () => {
        this.info = ' 連絡先を更新しました'
      })
    }
  }
}
</script>
